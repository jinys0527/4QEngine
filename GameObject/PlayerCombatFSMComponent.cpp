#include "PlayerCombatFSMComponent.h"
#include "CombatEvents.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"
#include "GridSystemComponent.h"
#include "ItemComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "GameObject.h"
#include "ServiceRegistry.h"
#include "CombatManager.h"
#include <algorithm>
#include "GameManager.h"
#include <cmath>
#include "CombatResolver.h"
#include "DiceSystem.h"
#include "LogSystem.h"
#include "TransformComponent.h"
#include <iostream>

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
		
			int range = 0;
			ItemComponent* throwItem = nullptr;
			bool isThrowMode = false;
			ResolvePlayerAttackMode(*player, range, throwItem, isThrowMode);

			const int cost = ResolveActionPointCost(*player, isThrowMode, throwItem);
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
			ExecutePlayerAttack();
		});

	BindActionHandler("Combat_Enter", [this](const FSMAction& action)
		{
			const int playerId = GetPlayerActorId();
			RequestCombatEnter(playerId, 0);
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

bool PlayerCombatFSMComponent::RequestCombatEnter(int initiatorId, int targetId)
{
	if (!EnsureCombatManager())
	{
		return false;
	}

	if (m_CombatManager->GetState() == Battle::InBattle)
	{
		if (m_CombatManager->GetCurrentActorId() == GetPlayerActorId())
		{
			DispatchEvent("Combat_StartTurn");
		}
		return true;
	}

	std::vector<CombatantSnapshot> combatants;
	BuildCombatantSnapshots(combatants);
	if (combatants.empty())
	{
		return false;
	}

	if (targetId == 0)
	{
		for (const auto& combatant : combatants)
		{
			if (!combatant.isPlayer)
			{
				targetId = combatant.actorId;
				break;
			}
		}
		if (targetId == 0)
		{
			return false;
		}
	}

	m_CombatManager->SetCombatants(combatants);
	GetEventDispatcher().Dispatch(EventType::CombatContextReady, nullptr);

	if (m_CombatManager->GetState() == Battle::NonBattle)
	{
		m_CombatManager->EnterBattle(initiatorId, targetId);
	}

	if (m_CombatManager->GetCurrentActorId() == GetPlayerActorId())
	{
		DispatchEvent("Combat_StartTurn");
	}

	return true;
}

bool PlayerCombatFSMComponent::TryExecutePlayerAttackFromInput()
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (!gameManager || gameManager->GetPhase() != Phase::TurnBasedCombat
		|| gameManager->GetCombatTurnState() != CombatTurnState::PlayerTurn)
	{
		return false;
	}

	auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
	if (!player)
	{
		return false;
	}

	int range = 0;
	ItemComponent* throwItem = nullptr;
	bool isThrowMode = false;
	ResolvePlayerAttackMode(*player, range, throwItem, isThrowMode);

	const int cost = ResolveActionPointCost(*player, isThrowMode, throwItem);
	if (!player->ConsumeActResource(cost))
	{
		return false;
	}

	return ExecutePlayerAttack();
}

bool PlayerCombatFSMComponent::TryExecutePlayerThrowAttack(EnemyComponent* enemy)
{
	auto* owner = GetOwner();
	auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
	if (!player || !enemy)
	{
		return false;
	}

	int throwRange = 0;
	ItemComponent* throwItem = nullptr;
	if (!player->TryGetConsumableThrowRange(throwRange) || !player->TryGetConsumableThrowItem(throwItem))
	{
		return false;
	}

	const int distance = AxialDistance(player->GetQ(), player->GetR(), enemy->GetQ(), enemy->GetR());
	if (distance > throwRange)
	{
		return false;
	}

	return ExecuteThrowAttack(*player, enemy, throwItem);
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

bool PlayerCombatFSMComponent::ExecutePlayerAttack()
{
	if (!EnsureCombatManager())
	{
		return false;
	}

	AttackRequest request;
	request.actorId = GetPlayerActorId();
	if (request.actorId == 0)
	{
		return false;
	}

	auto* owner = GetOwner();
	auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
	auto* grid = player ? player->GetGridSystem() : nullptr;
	ItemComponent* throwItem = nullptr;
	bool isThrowMode = false;
	if (grid && player)
	{
		int range = 0;
		ResolvePlayerAttackMode(*player, range, throwItem, isThrowMode);
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

	if (request.targetIds.empty())
	{
		return false;
	}

	auto* scene = owner ? owner->GetScene() : nullptr;
	EnemyComponent* enemy = nullptr;
	if (grid)
	{
		for (auto* candidate : grid->GetEnemies())
		{
			if (candidate && candidate->GetActorId() == request.targetIds.front())
			{
				enemy = candidate;
				break;
			}
		}
	}
	if (scene && enemy)
	{
		auto& services = scene->GetServices();
		if (player && isThrowMode)
		{
			if (!ExecuteThrowAttack(*player, enemy, throwItem))
			{
				return false;
			}
		}
		else if (services.Has<CombatResolver>() && services.Has<DiceSystem>()) 
		{
			auto* enemyOwner = enemy->GetOwner();
			auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
			auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;


			if (enemyStat && playerStat)
			{
				AttackProfile attackProfile{};
				attackProfile.attackModifier = playerStat->GetCalculatedStrengthModifier();
				attackProfile.allowCritical = true;
				attackProfile.autoFailOnOne = false;
				attackProfile.attackerName = "Player";
				attackProfile.targetName = "Enemy";

				DefenseProfile defenseProfile{};
				defenseProfile.defense = enemyStat->GetDefense();

				auto& resolver = services.Get<CombatResolver>();
				auto& diceSystem = services.Get<DiceSystem>();
				auto* logger = services.Has<LogSystem>() ? &services.Get<LogSystem>() : nullptr;

				std::cout << "[Combat] Player STR mod=" << attackProfile.attackModifier
					<< " Enemy DEF=" << defenseProfile.defense << std::endl;

				const int prevHp = enemyStat->GetCurrentHP();
				CombatRollResult result = resolver.ResolveAttack(attackProfile, defenseProfile, diceSystem, logger);
				if (result.hit != HitResult::Miss && result.damage > 0)
				{
					const int nextHp = max(0, prevHp - result.damage);
					enemyStat->SetCurrentHP(nextHp);
					std::cout << "[Combat] Enemy HP: " << prevHp << " -> " << nextHp << std::endl;
					if (enemyStat->IsDead() && m_CombatManager)
					{
						bool enemiesRemaining = false;
						if (grid)
						{
							for (auto* candidate : grid->GetEnemies())
							{
								if (!candidate)
								{
									continue;
								}
								auto* candidateOwner = candidate->GetOwner();
								auto* candidateStat = candidateOwner ? candidateOwner->GetComponent<EnemyStatComponent>() : nullptr;
								if (candidateStat && !candidateStat->IsDead())
								{
									enemiesRemaining = true;
									break;
								}
							}
						}

						const bool playerAlive = playerStat && !playerStat->IsDead();
						m_CombatManager->UpdateBattleOutcome(playerAlive, enemiesRemaining);
					}
				}
			}
		}
	}

	if (scene)
	{
		if (auto* gameManager = scene->GetGameManager())
		{
			if (gameManager->GetPhase() == Phase::ExplorationLoop)
			{
				GetEventDispatcher().Dispatch(EventType::PhaseRequestEnterCombat, nullptr);
			}
		}
	}

	m_CombatManager->HandlePlayerAttack(request);
	return true;
}

bool PlayerCombatFSMComponent::ResolvePlayerAttackMode(PlayerComponent& player, int& outRange, ItemComponent*& outThrowItem, bool& outIsThrow) const
{
	outRange = 0;
	outThrowItem = nullptr;
	outIsThrow = false;

	if (player.IsThrowPreviewActive())
	{
		int throwRange = 0;
		ItemComponent* throwItem = nullptr;
		if (player.TryGetConsumableThrowRange(throwRange) && player.TryGetConsumableThrowItem(throwItem))
		{
			outRange = throwRange;
			outThrowItem = throwItem;
			outIsThrow = true;
			return true;
		}
	}

	outRange = max(0, player.GetAttackRange());
	return true;
}

int PlayerCombatFSMComponent::ResolveActionPointCost(PlayerComponent& player, bool isThrowMode, ItemComponent* throwItem) const
{
	if (isThrowMode && throwItem)
	{
		return max(0, throwItem->GetActionPointCost());
	}

	return max(0, player.GetCurrentWeaponCost());
}

bool PlayerCombatFSMComponent::ExecuteThrowAttack(PlayerComponent& player, EnemyComponent* enemy, ItemComponent* throwItem)
{
	if (!enemy || !throwItem)
	{
		return false;
	}

	if (!ApplyThrowDamage(throwItem, enemy))
	{
		return false;
	}

	auto* owner = GetOwner();
	auto* playerTransform = owner ? owner->GetComponent<TransformComponent>() : nullptr;
	auto* enemyOwner = enemy->GetOwner();
	auto* enemyTransform = enemyOwner ? enemyOwner->GetComponent<TransformComponent>() : nullptr;

	XMFLOAT3 startPos = playerTransform ? playerTransform->GetPosition() : XMFLOAT3{};
	XMFLOAT3 targetPos = enemyTransform ? enemyTransform->GetPosition() : XMFLOAT3{};

	startPos.y += 1.0f;
	targetPos.y += 1.0f;

	throwItem->BeginThrow(startPos, targetPos, 2.0f);
	player.ConsumeThrowItem(throwItem);
	return true;
}

bool PlayerCombatFSMComponent::ApplyThrowDamage(ItemComponent* throwItem, EnemyComponent* enemy) const
{
	if (!throwItem || !enemy)
	{
		return false;
	}

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return false;
	}

	auto* enemyOwner = enemy->GetOwner();
	auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
	auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;
	if (!enemyStat || !playerStat)
	{
		return false;
	}

	auto& services = scene->GetServices();
	if (!services.Has<DiceSystem>())
	{
		return false;
	}

	auto& diceSystem = services.Get<DiceSystem>();
	const int diceCount = max(0, throwItem->GetDiceRoll());
	const int diceSides = max(0, throwItem->GetDiceType());
	const int bonus = throwItem->GetBaseModifier();
	const int agilityModifier = playerStat->GetCalculatedAgilityModifier();
	int damage = bonus + agilityModifier;

	if (diceCount > 0 && diceSides > 0)
	{
		const DiceConfig rollConfig{ diceCount, diceSides, 0 };
		damage += diceSystem.RollTotal(rollConfig, RandomDomain::World);
	}

	if (damage <= 0)
	{
		return false;
	}

	const int prevHp = enemyStat->GetCurrentHP();
	const int nextHp = max(0, prevHp - damage);
	enemyStat->SetCurrentHP(nextHp);
	std::cout << "[Throw] Damage=" << damage << " (AGI mod=" << agilityModifier
		<< ") Enemy HP: " << prevHp << " -> " << nextHp << std::endl;
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

	int range = 0;
	ItemComponent* throwItem = nullptr;
	bool isThrowMode = false;
	ResolvePlayerAttackMode(*player, range, throwItem, isThrowMode);
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
