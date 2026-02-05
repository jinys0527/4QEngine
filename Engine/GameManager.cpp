#include "pch.h"
#include <iostream>
#include "GameManager.h"
#include "CombatEvents.h"
#include "Scene.h"
#include "GameObject.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "PlayerFSMComponent.h"
#include "PlayerMoveFSMComponent.h"
#include "PlayerPushFSMComponent.h"
#include "PlayerCombatFSMComponent.h"
#include "PlayerInventoryFSMComponent.h"
#include "PlayerShopFSMComponent.h"
#include "PlayerDoorFSMComponent.h"
#include "GridSystemComponent.h"
#include "ServiceRegistry.h"
#include "CombatManager.h"
#include "GameDataRepository.h"
#include "LogSystem.h"
#include "RandomMachine.h"
#include "DiceSystem.h"
#include "ShopRoller.h"
#include "CombatResolver.h"
#include "EnemyStatComponent.h"
#include "FloodSystemComponent.h"
#include "FloodUIComponent.h"
#include <chrono>
#include <charconv>
#include "EnemyComponent.h"
#include <system_error>

GameManager::GameManager() :

	m_Turn(Turn::PlayerTurn)
	, m_BattleCheck(Battle::NonBattle)
	, m_Phase(Phase::None)
	, m_ExplorationTurnState(ExplorationTurnState::PlayerTurn)
	, m_CombatTurnState(CombatTurnState::SelectActor)
{
}

GameManager::~GameManager()
{

}

void GameManager::SetEventDispatcher(EventDispatcher& eventDispatcher)
{
	if (m_EventDispatcher == &eventDispatcher)
	{
		return;
	}
	UnregisterEventListeners();
	m_EventDispatcher = &eventDispatcher;
	RegisterEventListeners();
}

void GameManager::ClearEventDispatcher()
{
	UnregisterEventListeners();
	m_EventDispatcher = nullptr;
}

void GameManager::Reset()
{
	TurnReset();
}

void GameManager::SetServices(ServiceRegistry* services)
{
	m_Services = services;
}

void GameManager::SetActiveScene(Scene* scene)
{
	m_ActiveScene = scene;
	if (m_WaitingForFloorScene && m_ActiveScene)
	{
		RefreshGridSystem();
		m_FloorReadyPending = true;
		m_WaitingForFloorScene = false;
	}
}

void GameManager::SetDataSheetPaths(const DataSheetPaths& paths)
{
	m_DataPaths = paths;
}

void GameManager::SetFloorSceneNames(const std::vector<std::string>& names)
{
	m_FloorSceneNames = names;
}

void GameManager::Update(float deltaTime)
{
	if (m_Phase == Phase::ExplorationLoop && m_ExplorationTurnState == ExplorationTurnState::PlayerTurn)
	{
		m_ExplorationTurnElapsed += deltaTime;
		if (m_ExplorationTurnElapsed >= m_ExplorationTurnLimit)
		{
			if (m_EventDispatcher)
			{
				m_EventDispatcher->Dispatch(EventType::ExploreTurnEnded, nullptr);
			}
			m_ExplorationTurnElapsed = 0.0f;
		}
	}

	if (m_Phase == Phase::TurnBasedCombat && m_CombatTurnState == CombatTurnState::PlayerTurn)
	{
		m_CombatTurnElapsed += deltaTime;
		if (m_CombatTurnElapsed >= m_CombatTurnLimit)
		{
			if (m_EventDispatcher)
			{
				m_EventDispatcher->Dispatch(EventType::PlayerTurnEndRequested, nullptr);
			}
			m_CombatTurnElapsed = 0.0f;
		}
	}


	if (m_Phase == Phase::TurnBasedCombat && m_CombatTurnState == CombatTurnState::Resolve)
	{
		auto* combatManager = GetCombatManager();
		if (combatManager)
		{
			combatManager->AdvanceTurn();
			SetCombatTurnState(CombatTurnState::SelectActor);
		}
	}

	if (m_InitCompletePending)
	{
		m_InitCompletePending = false;
		if (m_EventDispatcher)
		{
			m_EventDispatcher->Dispatch(EventType::InitComplete, nullptr);
		}
	}

	if (m_FloorReadyPending)
	{
		m_FloorReadyPending = false;
		if (m_EventDispatcher)
		{
			m_EventDispatcher->Dispatch(EventType::FloorReady, nullptr);
		}
	}
}

void GameManager::OnEvent(EventType type, const void* data)
{
	(void)data;
	switch (type)
	{
	case EventType::AITurnEndRequested:
		if (m_Phase == Phase::TurnBasedCombat && m_Turn == Turn::EnemyTurn)
		{
			std::cout << "AITurnEndRequested\n";
			SetCombatTurnState(CombatTurnState::Resolve);
		}
		break;
	case EventType::CombatEnter:
		std::cout << "CombatEnter\n";
		m_BattleCheck = Battle::InBattle;
		SetPhase(Phase::CombatTrigger);
		break;
	case EventType::CombatExit:
		std::cout << "AITurnCombatExitEndRequested\n";
		if (m_BlockPostCombatShop)
		{
			break;
		}
		m_BattleCheck = Battle::NonBattle;
		SetPhase(Phase::CombatEnd);
		break;
	case EventType::CombatTurnAdvanced:
		std::cout << "CombatTurnAdvanced\n";
		if (m_BattleCheck == Battle::InBattle)
		{
			const auto* payload = static_cast<const CombatTurnAdvancedEvent*>(data);
			if (payload)
			{
				SyncTurnFromActorId(payload->actorId);
				if (m_Phase == Phase::TurnBasedCombat)
				{
					SetCombatTurnState(payload->actorId == 1 ? CombatTurnState::PlayerTurn
						: CombatTurnState::EnemyTurn);
				}
			}
		}
		break;
	case EventType::PlayerTurnEndRequested:
		std::cout << "PlayerTurnEndRequested\n";
		if (m_Phase == Phase::ExplorationLoop && m_ExplorationTurnState == ExplorationTurnState::PlayerTurn)
		{
			SetExplorationTurnState(ExplorationTurnState::EnemyStep);
			SetTurn(Turn::EnemyTurn);
		}
		else if (m_Phase == Phase::TurnBasedCombat && m_CombatTurnState == CombatTurnState::PlayerTurn)
		{
			SetCombatTurnState(CombatTurnState::Resolve);
		}
		break;
	case EventType::EnemyTurnEndRequested:
		std::cout << "EnemyTurnEndRequested\n";
		if (m_Phase == Phase::ExplorationLoop && m_ExplorationTurnState == ExplorationTurnState::EnemyStep)
		{
			SetExplorationTurnState(ExplorationTurnState::PlayerTurn);
			SetTurn(Turn::PlayerTurn);
		}
		else if (m_Phase == Phase::TurnBasedCombat && m_CombatTurnState == CombatTurnState::EnemyTurn)
		{
			SetCombatTurnState(CombatTurnState::Resolve);
		}
		break;
	case EventType::AIMeleeAttackRequested:
	case EventType::AIRangedAttackRequested:
		std::cout << "AIAttackRequested\n";
		if (m_Phase == Phase::TurnBasedCombat)
		{
			ResolveEnemyAttack();
			if (m_BlockPostCombatShop)
			{
				break;
			}
			SetCombatTurnState(CombatTurnState::Resolve);
		}
		break;
	case EventType::ExploreTurnEnded:
		std::cout << "ExploreTurnEnded\n";
		if (m_Phase == Phase::ExplorationLoop)
		{
			SetExplorationTurnState(ExplorationTurnState::EnemyStep);
			SetTurn(Turn::EnemyTurn);
		}
		break;
	case EventType::ExploreEnemyStepEnded:
		std::cout << "ExploreEnemyStepEnded\n";
		if (m_Phase == Phase::ExplorationLoop)
		{
			SetExplorationTurnState(ExplorationTurnState::PlayerTurn);
			SetTurn(Turn::PlayerTurn);
		}
		break;
	case EventType::PhaseRequestEnterCombat:
		std::cout << "PhaseRequestEnterCombat\n";
		SetPhase(Phase::CombatTrigger);
		break;
	case EventType::CombatContextReady:
		std::cout << "CombatContextReady\n";
		SetPhase(Phase::CombatInit);
		break;
	case EventType::CombatInitComplete:
		std::cout << "CombatInitComplete\n";
		SetPhase(Phase::TurnBasedCombat);
		SetCombatTurnState(CombatTurnState::SelectActor);
		break;
	case EventType::CombatEnded:
		std::cout << "CombatEnded\n";
		if (m_BlockPostCombatShop)
		{
			break;
		}
		SetPhase(Phase::CombatEnd);
		break;
	case EventType::PostCombatToShop:
		std::cout << "PostCombatToShop\n";
		if (m_BlockPostCombatShop || m_Phase == Phase::GameOver)
		{
			break;
		}
		SetPhase(Phase::Shop);
		break;
	case EventType::PostCombatToExploration:
		std::cout << "PostCombatToExploration\n";
		SetPhase(Phase::ExplorationLoop);
		SetExplorationTurnState(ExplorationTurnState::PlayerTurn);
		SetTurn(Turn::PlayerTurn);
		break;
	case EventType::ShopDone:
		std::cout << "ShopDone\n";
		SetPhase(Phase::NextFloor);
		break;
	case EventType::FloorReady:
		std::cout << "FloorReady\n";
		SetPhase(Phase::ExplorationLoop);
		SetExplorationTurnState(ExplorationTurnState::PlayerTurn);
		SetTurn(Turn::PlayerTurn);
		break;
	case EventType::GameStart:
		std::cout << "GameStart\n";
		SetPhase(Phase::InitCharacter);
		break;
	case EventType::InitComplete:
		std::cout << "InitComplete\n";
		SetPhase(Phase::ExplorationLoop);
		SetExplorationTurnState(ExplorationTurnState::PlayerTurn);
		SetTurn(Turn::PlayerTurn);
		break;
	case EventType::GameWin:
	case EventType::GameOver:
		std::cout << "GameWin or GameOver\n";
		m_BlockPostCombatShop = true;
		SetPhase(Phase::GameOver);
		break;
	default:
		break;
	}
}

void GameManager::TurnReset()
{
	SetTurn(Turn::PlayerTurn);
	m_BattleCheck = Battle::NonBattle;
	SetPhase(Phase::GameStart);
	SetExplorationTurnState(ExplorationTurnState::PlayerTurn);
	SetCombatTurnState(CombatTurnState::SelectActor);

	m_ExplorationTurnElapsed = 0.0f;
	m_CurrentFloor = 1;
	m_GameDataLoaded = false;
	m_WaitingForFloorScene = false;
	m_FloorReadyPending = false;
	m_BlockPostCombatShop = false;
}

void GameManager::Initial()
{
	TurnReset();
}

//Scene 변경 요청
void GameManager::RequestSceneChange(const std::string& name)
{
	if (!m_EventDispatcher)
	{
		return;
	}

	Events::SceneChangeRequest request{ name };
	m_EventDispatcher->Dispatch(EventType::SceneChangeRequested, &request);
}

void GameManager::SetTurn(Turn turn)
{
	if (m_Turn == turn)
	{
		return;
	}

	m_Turn = turn;
	DispatchTurnChanged();
}

bool GameManager::IsExplorationInputAllowed() const
{
	return m_Phase == Phase::ExplorationLoop
		&& m_ExplorationTurnState == ExplorationTurnState::PlayerTurn;
}

bool GameManager::IsCombatInputAllowed() const
{
	return m_Phase == Phase::TurnBasedCombat
		&& m_CombatTurnState == CombatTurnState::PlayerTurn;
}

bool GameManager::IsShopInputAllowed() const
{
	return m_Phase == Phase::Shop;
}

void GameManager::SetPhase(Phase phase)
{
	if (m_Phase == phase)
	{
		return;
	}

	const Phase previous = m_Phase;
	OnPhaseExit(previous);
	m_Phase = phase;
	OnPhaseEnter(phase);
}

void GameManager::SetExplorationTurnState(ExplorationTurnState state)
{
	if (m_ExplorationTurnState == state)
	{
		return;
	}

	const ExplorationTurnState previous = m_ExplorationTurnState;
	OnExplorationTurnStateExit(previous);
	m_ExplorationTurnState = state;
	OnExplorationTurnStateEnter(state);
}

void GameManager::SetCombatTurnState(CombatTurnState state)
{
	if (m_CombatTurnState == state)
	{
		return;
	}

	const CombatTurnState previous = m_CombatTurnState;
	OnCombatTurnStateExit(previous);
	m_CombatTurnState = state;
	OnCombatTurnStateEnter(state);
}

void GameManager::OnPhaseEnter(Phase phase)
{
	switch (phase)
	{
	case Phase::GameStart:
		if (auto* randomMachine = GetRandomMachine())
		{
			const auto now = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
			randomMachine->SetSeed(now);
		}
		if (auto* logger = GetLogSystem())
		{
			logger->Clear();
			logger->Add(LogChannel::System, "GameStart: systems initialized.");
		}
		LoadGameData();
		if (m_EventDispatcher)
		{
			m_EventDispatcher->Dispatch(EventType::GameStart, nullptr);
		}
		break;
	case Phase::InitCharacter:
		m_CurrentFloor = 1;
		InitializePlayer();
		InitializeFloor();
		m_InitCompletePending = true;
		if (auto* logger = GetLogSystem())
		{
			logger->Add(LogChannel::System, "InitCharacter: player initialized.");
		}
		break;
	case Phase::ExplorationLoop:
		SetExplorationTurnState(ExplorationTurnState::PlayerTurn);
		SetTurn(Turn::PlayerTurn);
		m_ExplorationTurnElapsed = 0.0f;
		SetPlayerShopState(false);
		SetFloodSystemActive(true);
		break;
	case Phase::CombatTrigger:
		SetFloodSystemActive(false);
		SetCombatTurnState(CombatTurnState::SelectActor);
		break;
	case Phase::CombatInit:
		SetFloodSystemActive(false);
		SetCombatTurnState(CombatTurnState::SelectActor);
		DispatchPlayerFSMEvent("CombatInitComplete");
		break;
	case Phase::TurnBasedCombat:
		SetFloodSystemActive(false);
		SetCombatTurnState(CombatTurnState::SelectActor);
		DispatchPlayerFSMEvent("Combat_Start");
		break;
	case Phase::CombatEnd:
		SetCombatTurnState(CombatTurnState::SelectActor);
		SetFloodSystemActive(false);
		DispatchPlayerFSMEvent("Combat_End");
		if (auto* logger = GetLogSystem())
		{
			logger->Add(LogChannel::Combat, "Combat ended. Resolving cleanup.");
		}
		if (m_EventDispatcher && !m_BlockPostCombatShop)
		{
			m_EventDispatcher->Dispatch(EventType::PostCombatToShop, nullptr);
		}
		break;
	case Phase::Shop:
		SetTurn(Turn::PlayerTurn);
		SetPlayerShopState(true);
		SetFloodSystemActive(false);
		DispatchPlayerFSMEvent("Shop_Open");
		{
			auto* repository = GetGameDataRepository();
			auto* diceSystem = GetDiceSystem();
			auto* shopRoller = GetShopRoller();
			if (repository && diceSystem && shopRoller)
			{
				const std::vector<int> ownedItems = CollectOwnedItemIndices();
				m_CurrentShopStock = shopRoller->RollStock(m_CurrentFloor, *repository, *diceSystem, ownedItems, GetLogSystem());
				if (auto* logger = GetLogSystem())
				{
					if (!m_CurrentShopStock.equipmentItems.empty())
					{
						const auto* item = repository->GetItem(m_CurrentShopStock.equipmentItems.front());
						if (item)
						{
							logger->Add(LogChannel::Shop, "Shop item: " + item->name);
						}
					}
					if (!m_CurrentShopStock.consumableItems.empty())
					{
						const auto* item = repository->GetItem(m_CurrentShopStock.consumableItems.front());
						if (item)
						{
							logger->Add(LogChannel::Shop, "Shop consumable: " + item->name);
						}
					}
				}
			}
		}
		if (auto* logger = GetLogSystem())
		{
			logger->Add(LogChannel::Shop, "Shop opened.");
		}
		break;
	case Phase::NextFloor:
		SetTurn(Turn::PlayerTurn);
		AdvanceFloor();
		{
			const std::string* targetScene = nullptr;
			if (!m_FloorSceneNames.empty() && m_CurrentFloor > 0)
			{
				const size_t index = static_cast<size_t>(m_CurrentFloor - 1);
				if (index < m_FloorSceneNames.size())
				{
					targetScene = &m_FloorSceneNames[index];
				}
			}

			if (targetScene && (!m_ActiveScene || m_ActiveScene->GetName() != *targetScene))
			{
				RequestSceneChange(*targetScene);
				m_WaitingForFloorScene = true;
			}
			else if (!targetScene && !m_FloorSceneNames.empty())
			{
				if (m_EventDispatcher)
				{
					m_EventDispatcher->Dispatch(EventType::GameWin, nullptr);
				}
			}
			else
			{
				RefreshGridSystem();
				m_FloorReadyPending = true;
			}
		}
		if (auto* logger = GetLogSystem())
		{
			logger->Add(LogChannel::System, "NextFloor: floor advanced.");
		}
		break;
	case Phase::GameOver:
		SetFloodSystemActive(false);
		if (auto* logger = GetLogSystem())
		{
			logger->Add(LogChannel::System, "GameOver: session ended.");
		}
		break;
	}
}

void GameManager::OnPhaseExit(Phase phase)
{
	if (phase == Phase::Shop)
	{
		SetPlayerShopState(false);
		DispatchPlayerFSMEvent("Shop_Close");
	}
	if (phase == Phase::ExplorationLoop)
	{
		SetFloodSystemActive(false);
	}
}

void GameManager::OnExplorationTurnStateEnter(ExplorationTurnState state)
{
	switch (state)
	{
	case ExplorationTurnState::PlayerTurn:
		m_ExplorationTurnElapsed = 0.0f;
		SetTurn(Turn::PlayerTurn);
		break;
	case ExplorationTurnState::EnemyStep:
		m_ExplorationTurnElapsed = 0.0f;
		SetTurn(Turn::EnemyTurn);
		break;
	}
}

void GameManager::OnExplorationTurnStateExit(ExplorationTurnState state)
{
	(void)state;
}

void GameManager::OnCombatTurnStateEnter(CombatTurnState state)
{
	if (state == CombatTurnState::SelectActor)
	{
		auto* combatManager = GetCombatManager();
		if (!combatManager)
		{
			return;
		}

		const int actorId = combatManager->GetCurrentActorId();
		if (actorId == 1)
		{
			SetCombatTurnState(CombatTurnState::PlayerTurn);
		}
		else if (actorId != 0)
		{
			SetCombatTurnState(CombatTurnState::EnemyTurn);
		}
	}
	else if (state == CombatTurnState::PlayerTurn)
	{
		m_CombatTurnElapsed = 0.0f;
	}
}

void GameManager::OnCombatTurnStateExit(CombatTurnState state)
{
	(void)state;
}

CombatManager* GameManager::GetCombatManager() const
{
	if (!m_Services || !m_Services->Has<CombatManager>())
	{
		return nullptr;
	}
	return &m_Services->Get<CombatManager>();
}

GameDataRepository* GameManager::GetGameDataRepository() const
{
	if (!m_Services || !m_Services->Has<GameDataRepository>())
	{
		return nullptr;
	}
	return &m_Services->Get<GameDataRepository>();
}

LogSystem* GameManager::GetLogSystem() const
{
	if (!m_Services || !m_Services->Has<LogSystem>())
	{
		return nullptr;
	}
	return &m_Services->Get<LogSystem>();
}

RandomMachine* GameManager::GetRandomMachine() const
{
	if (!m_Services || !m_Services->Has<RandomMachine>())
	{
		return nullptr;
	}
	return &m_Services->Get<RandomMachine>();
}

DiceSystem* GameManager::GetDiceSystem() const
{
	if (!m_Services || !m_Services->Has<DiceSystem>())
	{
		return nullptr;
	}
	return &m_Services->Get<DiceSystem>();
}

ShopRoller* GameManager::GetShopRoller() const
{
	if (!m_Services || !m_Services->Has<ShopRoller>())
	{
		return nullptr;
	}
	return &m_Services->Get<ShopRoller>();
}

void GameManager::LoadGameData()
{
	if (m_GameDataLoaded)
	{
		return;
	}

	auto* repository = GetGameDataRepository();
	if (!repository)
	{
		return;
	}

	if (m_DataPaths.itemsPath.empty() || m_DataPaths.enemiesPath.empty() || m_DataPaths.dropTablesPath.empty())
	{
		auto* logger = GetLogSystem();
		if (logger)
		{
			logger->Add(LogChannel::System, "Game data paths are not configured.");
		}
		return;
	}

	std::string error;
	if (!repository->LoadFromFiles(m_DataPaths, &error))
	{
		auto* logger = GetLogSystem();
		if (logger)
		{
			logger->Add(LogChannel::System, "Failed to load game data: " + error);
		}
		return;
	}

	m_GameDataLoaded = true;
}

void GameManager::InitializePlayer()
{
	if (!m_ActiveScene)
	{
		return;
	}

	auto* playerObject = FindPlayerObject(m_ActiveScene);
	if (!playerObject)
	{
		return;
	}

	auto* player = playerObject->GetComponent<PlayerComponent>();
	auto* stats = playerObject->GetComponent<PlayerStatComponent>();
	if (!player || !stats)
	{
		return;
	}

	stats->SetHealth(12);
	stats->SetStrength(12);
	stats->SetAgility(12);
	stats->SetSense(12);
	stats->SetSkill(12);
	stats->SetEquipmentDefenseBonus(0);

	const int maxHp = stats->GetMaxHealthForFloor(m_CurrentFloor);
	stats->SetCurrentHP(maxHp);

	player->SetActorId(1);
	player->SetMoney(0);
	player->SetInventoryItemIds({});
	player->ResetTurnResources();
}

void GameManager::InitializeFloor()
{
	m_CurrentFloor = 1;
	AdvanceFloor();
	RefreshGridSystem();
}

void GameManager::AdvanceFloor()
{
	if (!m_ActiveScene)
	{
		return;
	}

	if (m_Phase == Phase::NextFloor)
	{
		++m_CurrentFloor;
	}

	auto* playerObject = FindPlayerObject(m_ActiveScene);
	if (!playerObject)
	{
		return;
	}

	auto* stats = playerObject->GetComponent<PlayerStatComponent>();
	auto* player = playerObject->GetComponent<PlayerComponent>();
	if (!stats || !player)
	{
		return;
	}

	const int maxHp = stats->GetMaxHealthForFloor(m_CurrentFloor);
	stats->SetCurrentHP(maxHp);
	player->ResetTurnResources();
}

void GameManager::SetPlayerShopState(bool inShop)
{
	if (!m_ActiveScene)
	{
		return;
	}

	auto* playerObject = FindPlayerObject(m_ActiveScene);
	if (!playerObject)
	{
		return;
	}

	auto* player = playerObject->GetComponent<PlayerComponent>();
	if (!player)
	{
		return;
	}

	player->SetInventoryAtShop(inShop);
	player->SetInventoryCanDrop(!inShop);
	player->SetShopHasSpace(true);
	player->SetShopHasMoney(true);
}

void GameManager::SetFloodSystemActive(bool active)
{
	if (!m_ActiveScene)
	{
		return;
	}

	for (const auto& [name, object] : m_ActiveScene->GetGameObjects())
	{
		(void)name;
		if (!object)
		{
			continue;
		}

		for (auto* floodSystem : object->GetComponents<FloodSystemComponent>())
		{
			floodSystem->SetIsActive(active);
		}

		for (auto* floodUi : object->GetComponents<FloodUIComponent>())
		{
			floodUi->SetIsActive(active);
		}
	}
}

void GameManager::RefreshGridSystem()
{
	if (!m_ActiveScene)
	{
		return;
	}

	for (const auto& [name, object] : m_ActiveScene->GetGameObjects())
	{
		(void)name;
		if (!object)
		{
			continue;
		}

		if (auto* grid = object->GetComponent<GridSystemComponent>())
		{
			grid->ScanNodes();
			grid->MakeGraph();
		}
	}
}

void GameManager::DispatchPlayerFSMEvent(const std::string& eventName)
{
	if (!m_ActiveScene || eventName.empty())
	{
		return;
	}

	auto* playerObject = FindPlayerObject(m_ActiveScene);
	if (!playerObject)
	{
		return;
	}

	if (auto* playerFsm = playerObject->GetComponent<PlayerFSMComponent>())
	{
		playerFsm->DispatchEvent(eventName);
	}
}

void GameManager::ResolveEnemyAttack()
{
	if (!m_ActiveScene)
	{
		return;
	}

	auto* combatManager = GetCombatManager();
	if (!combatManager)
	{
		return;
	}

	const int actorId = combatManager->GetCurrentActorId();
	if (actorId == 0 || actorId == 1)
	{
		return;
	}

	auto* playerObject = FindPlayerObject(m_ActiveScene);
	if (!playerObject)
	{
		return;
	}

	auto* playerStat = playerObject->GetComponent<PlayerStatComponent>();
	if (!playerStat)
	{
		return;
	}

	GridSystemComponent* grid = nullptr;
	for (const auto& [name, object] : m_ActiveScene->GetGameObjects())
	{
		(void)name;
		if (!object)
		{
			continue;
		}
		if (auto* candidate = object->GetComponent<GridSystemComponent>())
		{
			grid = candidate;
			break;
		}
	}
	if (!grid)
	{
		return;
	}

	EnemyComponent* enemy = nullptr;
	for (auto* candidate : grid->GetEnemies())
	{
		if (candidate && candidate->GetActorId() == actorId)
		{
			enemy = candidate;
			break;
		}
	}

	if (!enemy)
	{
		return;
	}

	auto* enemyOwner = enemy->GetOwner();
	auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
	if (!enemyStat)
	{
		return;
	}

	auto* diceSystem = GetDiceSystem();
	auto* resolver = m_Services && m_Services->Has<CombatResolver>() ? &m_Services->Get<CombatResolver>() : nullptr;
	if (!diceSystem || !resolver)
	{
		return;
	}

	AttackProfile attackProfile{};
	attackProfile.attackModifier = enemyStat->GetAccuracyModifier();
	attackProfile.allowCritical = true;
	attackProfile.autoFailOnOne = true;
	attackProfile.attackerName = "Enemy";
	attackProfile.targetName = "Player";
	attackProfile.minDamage = enemyStat->GetDiceRollCount();
	attackProfile.maxDamage = enemyStat->GetDiceRollCount() * enemyStat->GetMaxDiceValue();

	DefenseProfile defenseProfile{};
	defenseProfile.defense = playerStat->GetDefense();

	const int prevHp = playerStat->GetCurrentHP();
	auto* logger = GetLogSystem();
	std::cout << "[Combat] Enemy ACC mod=" << attackProfile.attackModifier
		<< " Player DEF=" << defenseProfile.defense << std::endl;
	CombatRollResult result = resolver->ResolveAttack(attackProfile, defenseProfile, *diceSystem, logger);
	if (result.hit != HitResult::Miss && result.damage > 0)
	{
		const int nextHp = std::max(0, prevHp - result.damage);
		playerStat->SetCurrentHP(nextHp);
		std::cout << "[Combat] Player HP: " << prevHp << " -> " << nextHp << std::endl;
		if (nextHp <= 0)
		{
			m_BlockPostCombatShop = true;
			if (m_Services && m_Services->Has<CombatManager>())
			{
				m_Services->Get<CombatManager>().UpdateBattleOutcome(false, true);
			}
			if (m_EventDispatcher && m_Phase != Phase::GameOver)
			{
				m_EventDispatcher->Dispatch(EventType::GameOver, nullptr);
			}
		}
	}
}

std::vector<int> GameManager::CollectOwnedItemIndices() const
{
	std::vector<int> owned;
	if (!m_ActiveScene)
	{
		return owned;
	}

	auto* playerObject = FindPlayerObject(m_ActiveScene);
	if (!playerObject)
	{
		return owned;
	}

	auto* player = playerObject->GetComponent<PlayerComponent>();
	if (!player)
	{
		return owned;
	}

	for (const auto& itemId : player->GetInventoryItemIds())
	{
		int value = 0;
		const char* begin = itemId.data();
		const char* end = begin + itemId.size();
		const auto result = std::from_chars(begin, end, value);
		if (result.ec == std::errc() && value > 0)
		{
			owned.push_back(value);
		}
	}

	return owned;
}

void GameManager::CapturePlayerData(Scene* scene)
{
	auto* playerObject = FindPlayerObject(scene);
	if (!playerObject)
	{
		return;
	}

	auto* player = playerObject->GetComponent<PlayerComponent>();
	auto* stat = playerObject->GetComponent<PlayerStatComponent>();
	if (!player || !stat)
	{
		return;
	}

	m_PlayerData.hasData = true;
	m_PlayerData.weaponCost = player->GetCurrentWeaponCost();
	m_PlayerData.attackRange = player->GetAttackRange();
	m_PlayerData.actorId = player->GetActorId();
	m_PlayerData.money = player->GetMoney();
	m_PlayerData.inventoryItemIds = player->GetInventoryItemIds();

	m_PlayerData.currentHP = stat->GetCurrentHP();
	m_PlayerData.health = stat->GetHealth();
	m_PlayerData.strength = stat->GetStrength();
	m_PlayerData.agility = stat->GetAgility();
	m_PlayerData.sense = stat->GetSense();
	m_PlayerData.skill = stat->GetSkill();
	m_PlayerData.equipmentDefenseBonus = stat->GetEquipmentDefenseBonus();
}

void GameManager::ApplyPlayerData(Scene* scene)
{
	if (!m_PlayerData.hasData)
	{
		return;
	}

	auto* playerObject = FindPlayerObject(scene);
	if (!playerObject)
	{
		return;
	}

	auto* player = playerObject->GetComponent<PlayerComponent>();
	auto* stat = playerObject->GetComponent<PlayerStatComponent>();
	if (!player || !stat)
	{
		return;
	}

	player->SetCurrentWeaponCost(m_PlayerData.weaponCost);
	player->SetAttackRange(m_PlayerData.attackRange);
	player->SetActorId(m_PlayerData.actorId);
	player->SetMoney(m_PlayerData.money);
	player->SetInventoryItemIds(m_PlayerData.inventoryItemIds);

	stat->SetCurrentHP(m_PlayerData.currentHP);
	stat->SetHealth(m_PlayerData.health);
	stat->SetStrength(m_PlayerData.strength);
	stat->SetAgility(m_PlayerData.agility);
	stat->SetSense(m_PlayerData.sense);
	stat->SetSkill(m_PlayerData.skill);
	stat->SetEquipmentDefenseBonus(m_PlayerData.equipmentDefenseBonus);
}

void GameManager::RegisterEventListeners()
{
	if (!m_EventDispatcher)
		return;

	m_EventDispatcher->AddListener(EventType::AITurnEndRequested, this);
	m_EventDispatcher->AddListener(EventType::AIMeleeAttackRequested, this);
	m_EventDispatcher->AddListener(EventType::AIRangedAttackRequested, this);
	m_EventDispatcher->AddListener(EventType::PlayerTurnEndRequested, this);
	m_EventDispatcher->AddListener(EventType::EnemyTurnEndRequested, this);
	m_EventDispatcher->AddListener(EventType::CombatEnter, this);
	m_EventDispatcher->AddListener(EventType::CombatExit, this);
	m_EventDispatcher->AddListener(EventType::CombatTurnAdvanced, this);
	m_EventDispatcher->AddListener(EventType::CombatContextReady, this);
	m_EventDispatcher->AddListener(EventType::CombatInitComplete, this);
	m_EventDispatcher->AddListener(EventType::CombatEnded, this);
	m_EventDispatcher->AddListener(EventType::ExploreTurnEnded, this);
	m_EventDispatcher->AddListener(EventType::ExploreEnemyStepEnded, this);
	m_EventDispatcher->AddListener(EventType::PhaseRequestEnterCombat, this);
	m_EventDispatcher->AddListener(EventType::PostCombatToShop, this);
	m_EventDispatcher->AddListener(EventType::PostCombatToExploration, this);
	m_EventDispatcher->AddListener(EventType::ShopDone, this);
	m_EventDispatcher->AddListener(EventType::FloorReady, this);
	m_EventDispatcher->AddListener(EventType::GameStart, this);
	m_EventDispatcher->AddListener(EventType::InitComplete, this);
	m_EventDispatcher->AddListener(EventType::GameWin, this);
	m_EventDispatcher->AddListener(EventType::GameOver, this);

	DispatchTurnChanged();
}

void GameManager::UnregisterEventListeners()
{
	if (!m_EventDispatcher)
		return;

	m_EventDispatcher->RemoveListener(EventType::AITurnEndRequested, this);
	m_EventDispatcher->RemoveListener(EventType::AIMeleeAttackRequested, this);
	m_EventDispatcher->RemoveListener(EventType::AIRangedAttackRequested, this);
	m_EventDispatcher->RemoveListener(EventType::PlayerTurnEndRequested, this);
	m_EventDispatcher->RemoveListener(EventType::EnemyTurnEndRequested, this);
	m_EventDispatcher->RemoveListener(EventType::CombatEnter, this);
	m_EventDispatcher->RemoveListener(EventType::CombatExit, this);
	m_EventDispatcher->RemoveListener(EventType::CombatTurnAdvanced, this);
	m_EventDispatcher->RemoveListener(EventType::CombatContextReady, this);
	m_EventDispatcher->RemoveListener(EventType::CombatInitComplete, this);
	m_EventDispatcher->RemoveListener(EventType::CombatEnded, this);
	m_EventDispatcher->RemoveListener(EventType::ExploreTurnEnded, this);
	m_EventDispatcher->RemoveListener(EventType::ExploreEnemyStepEnded, this);
	m_EventDispatcher->RemoveListener(EventType::PhaseRequestEnterCombat, this);
	m_EventDispatcher->RemoveListener(EventType::PostCombatToShop, this);
	m_EventDispatcher->RemoveListener(EventType::PostCombatToExploration, this);
	m_EventDispatcher->RemoveListener(EventType::ShopDone, this);
	m_EventDispatcher->RemoveListener(EventType::FloorReady, this);
	m_EventDispatcher->RemoveListener(EventType::GameStart, this);
	m_EventDispatcher->RemoveListener(EventType::InitComplete, this);
	m_EventDispatcher->RemoveListener(EventType::GameWin, this);
	m_EventDispatcher->RemoveListener(EventType::GameOver, this);
}

void GameManager::DispatchTurnChanged()
{
	if (!m_EventDispatcher)
	{
		return;
	}

	Events::TurnChanged payload{ static_cast<int>(m_Turn) };
	m_EventDispatcher->Dispatch(EventType::TurnChanged, &payload);
}

void GameManager::SyncTurnFromActorId(int actorId)
{
	if (actorId == 1)
	{
		SetTurn(Turn::PlayerTurn);
	}
	else if (actorId != 0)
	{
		SetTurn(Turn::EnemyTurn);
	}
}

GameObject* GameManager::FindPlayerObject(Scene* scene) const
{
	if (!scene)
	{
		return nullptr;
	}

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		(void)name;
		if (object && object->GetComponent<PlayerComponent>())
		{
			return object.get();
		}
	}

	return nullptr;
}
