#include "PlayerCombatFSMComponent.h"
#include "CombatEvents.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"
#include "GridSystemComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "GameObject.h"
#include "ServiceRegistry.h"
#include "CombatManager.h"
#include <algorithm>
#include <cmath>

REGISTER_COMPONENT_DERIVED(PlayerCombatFSMComponent, FSMComponent)

namespace
{
	int AxialDistance(int q1, int r1, int q2, int r2)
	{
		const int dq = q1 - q2;
		const int dr = r1 - r2;
		const int ds = dq + dr;
		return (std::abs(dq) + std::abs(dr) + std::abs(ds)) / 2;
	}
}

PlayerCombatFSMComponent::PlayerCombatFSMComponent()
{
	BindActionHandler("Combat_ConsumeActResource", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				DispatchEvent("Combat_CostFail");
				return;
			}
		
			const int cost = player->GetCurrentWeaponCost();
			const bool consumed = player->ConsumeActResource(cost);
			DispatchEvent(consumed ? "Combat_CostOk" : "Combat_CostFail");
		});

	BindActionHandler("Combat_RangeCheck", [this](const FSMAction& action)
		{
			const bool inRange = HasEnemyInAttackRange();
			DispatchEvent(inRange ? "Combat_RangeOk" : "Combat_RangeFail");
		});

	BindActionHandler("Combat_Confirm", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				DispatchEvent("Combat_Cancel");
				return;
			}
			const bool confirmed = player->ConsumeCombatConfirmRequest();
			DispatchEvent(confirmed ? "Combat_Confirm" : "Combat_Cancel");
		});

	BindActionHandler("Combat_Attack", [this](const FSMAction& action)
		{
			DispatchEvent("Combat_StartTurn");
			
			if (!EnsureCombatManager())
			{
				return;
			}

			AttackRequest request;
			request.actorId = GetPlayerActorId();
			if (request.actorId == 0)
			{
				return;
			}

			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			auto* grid = player ? player->GetGridSystem() : nullptr;
			if (grid && player)
			{
				const int range = max(0, player->GetAttackRange());
				const int playerQ = player->GetQ();
				const int playerR = player->GetR();
				const auto& enemies = grid->GetEnemies();
				for (std::size_t index = 0; index < enemies.size(); ++index)
				{
					const auto* enemy = enemies[index];
					if (!enemy)
					{
						continue;
					}

					const int distance = AxialDistance(playerQ, playerR, enemy->GetQ(), enemy->GetR());
					if (distance <= range && enemy->GetActorId() != 0)
					{
						request.targetIds.push_back(enemy->GetActorId());
						break;
					}
				}
			}

			if (!request.targetIds.empty())
			{
				m_CombatManager->HandlePlayerAttack(request);
			}
		});

	BindActionHandler("Combat_Enter", [this](const FSMAction& action)
		{
			if (!EnsureCombatManager())
			{
				return;
			}

			std::vector<CombatantSnapshot> combatants;
			BuildCombatantSnapshots(combatants);
			if (combatants.empty())
			{
				return;
			}

			m_CombatManager->SetCombatants(combatants);

			const int playerId = GetPlayerActorId();
			int targetId = 0;
			for (const auto& combatant : combatants)
			{
				if (!combatant.isPlayer)
				{
					targetId = combatant.actorId;
					break;
				}
			}

			if (m_CombatManager->GetState() == Battle::NonBattle)
			{
				m_CombatManager->EnterBattle(playerId, targetId);
			}

			if (m_CombatManager->GetCurrentActorId() == playerId)
			{
				DispatchEvent("Combat_StartTurn");
			}
		});

	BindActionHandler("Combat_Result", [this](const FSMAction& action)
		{
			if (!m_CombatManager)
			{
				DispatchEvent("Combat_TurnResolved");
				return;
			}

			if (m_CombatManager->GetCurrentActorId() == GetPlayerActorId())
			{
				DispatchEvent("Combat_StartTurn");
			}
			else
			{
				DispatchEvent("Combat_TurnResolved");
			}
		});


	BindActionHandler("Combat_Exit", [this](const FSMAction&)
		{
			if (m_CombatManager)
			{
				m_CombatManager->ExitBattle();
			}
			const CombatExitEvent eventData;
			GetEventDispatcher().Dispatch(EventType::CombatExit, &eventData);
		});
}

PlayerCombatFSMComponent::~PlayerCombatFSMComponent()
{
	GetEventDispatcher().RemoveListener(EventType::CombatInitiativeBuilt, this);
	GetEventDispatcher().RemoveListener(EventType::CombatTurnAdvanced, this);
}

void PlayerCombatFSMComponent::Start()
{
	FSMComponent::Start();
	GetEventDispatcher().AddListener(EventType::CombatInitiativeBuilt, this);
	GetEventDispatcher().AddListener(EventType::CombatTurnAdvanced, this);
}

std::optional<std::string> PlayerCombatFSMComponent::TranslateEvent(EventType type, const void* data)
{
	if ((type != EventType::CombatInitiativeBuilt && type != EventType::CombatTurnAdvanced) || !data)
	{
		return std::nullopt;
	}

	int actorId = 0;
	if (type == EventType::CombatInitiativeBuilt)
	{
		const auto* payload = static_cast<const CombatInitiativeBuiltEvent*>(data);
		if (!payload || !payload->initiativeOrder || payload->initiativeOrder->empty())
		{
			return std::nullopt;
		}
		actorId = payload->initiativeOrder->front();
	}
	else
	{
		const auto* payload = static_cast<const CombatTurnAdvancedEvent*>(data);
		if (!payload)
		{
			return std::nullopt;
		}
		actorId = payload->actorId;
	}

	if (IsPlayerActor(actorId))
	{
		return std::string("Combat_StartTurn");
	}

	return std::string("Combat_TurnResolved");
}

bool PlayerCombatFSMComponent::EnsureCombatManager()
{
	if (m_CombatManager)
	{
		m_CombatManager->SetEventDispatcher(&GetEventDispatcher());
		return true;
	}

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return false;
	}

	auto& services = scene->GetServices();
	if (!services.Has<CombatManager>())
	{
		return false;
	}

	m_CombatManager = &services.Get<CombatManager>();
	m_CombatManager->SetEventDispatcher(&GetEventDispatcher());
	return true;
}

void PlayerCombatFSMComponent::BuildCombatantSnapshots(std::vector<CombatantSnapshot>& outCombatants) const
{
	outCombatants.clear();

	auto* owner = GetOwner();
	auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
	if (!player)
	{
		return;
	}

	int playerInitiative = 0;
	if (auto* stat = owner->GetComponent<PlayerStatComponent>())
	{
		playerInitiative = stat->GetCalculatedAgilityModifier();
	}

	outCombatants.push_back({ GetPlayerActorId(), playerInitiative, true });

	auto* grid = player->GetGridSystem();
	if (!grid)
	{
		return;
	}

	const auto& enemies = grid->GetEnemies();
	for (std::size_t index = 0; index < enemies.size(); ++index)
	{
		auto* enemy = enemies[index];
		if (!enemy)
		{
			continue;
		}

		int initiative = 0;
		auto* enemyOwner = enemy->GetOwner();
		if (enemyOwner)
		{
			if (auto* stat = enemyOwner->GetComponent<EnemyStatComponent>())
			{
				initiative = stat->GetInitiativeModifier();
			}
		}

		outCombatants.push_back({ enemy->GetActorId(), initiative, false });
	}
}

bool PlayerCombatFSMComponent::HasEnemyInAttackRange() const
{
	auto* owner = GetOwner();
	auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
	if (!player)
	{
		return false;
	}

	auto* grid = player->GetGridSystem();
	if (!grid)
	{
		return false;
	}

	const int range = max(0, player->GetAttackRange());
	const int playerQ = player->GetQ();
	const int playerR = player->GetR();
	const auto& enemies = grid->GetEnemies();
	for (const auto* enemy : enemies)
	{
		if (!enemy)
		{
			continue;
		}

		const int distance = AxialDistance(playerQ, playerR, enemy->GetQ(), enemy->GetR());
		if (distance <= range)
		{
			return true;
		}
	}

	return false;
}

int PlayerCombatFSMComponent::GetPlayerActorId() const
{
	auto* owner = GetOwner();
	auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
	return player ? player->GetActorId() : 0;
}

bool PlayerCombatFSMComponent::IsPlayerActor(int actorId) const
{
	return actorId != 0 && actorId == GetPlayerActorId();
}
