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

GameManager::GameManager() :

	m_Turn(Turn::PlayerTurn)
	, m_BattleCheck(Battle::NonBattle)
	, m_Phase(Phase::PlayerMove)
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

void GameManager::OnEvent(EventType type, const void* data)
{
	(void)data;
	switch (type)
	{
	case EventType::AITurnEndRequested:
		if (m_Turn == Turn::EnemyTurn)
		{
			SetTurn(Turn::PlayerTurn);
			m_Phase = Phase::PlayerMove;
		}
		break;
	case EventType::CombatEnter:
		m_BattleCheck = Battle::InBattle;
		break;
	case EventType::CombatExit:
		m_BattleCheck = Battle::NonBattle;
		break;
	case EventType::CombatTurnAdvanced:
		if (m_BattleCheck == Battle::InBattle)
		{
			const auto* payload = static_cast<const CombatTurnAdvancedEvent*>(data);
			if (payload)
			{
				SyncTurnFromActorId(payload->actorId);
			}
		}
		break;
	case EventType::PlayerTurnEndRequested:
		if (m_Turn == Turn::PlayerTurn)
		{
			SetTurn(Turn::EnemyTurn);
		}
		break;
	case EventType::EnemyTurnEndRequested:
		if (m_Turn == Turn::EnemyTurn)
		{
			SetTurn(Turn::PlayerTurn);
			m_Phase = Phase::PlayerMove;
		}
		break;
	case EventType::AIMeleeAttackRequested:
	case EventType::AIRangedAttackRequested:
		m_Phase = Phase::Attack;
		break;
	default:
		break;
	}
}

void GameManager::TurnReset()
{
	SetTurn(Turn::PlayerTurn);
	m_BattleCheck = Battle::NonBattle;
	m_Phase = Phase::PlayerMove;
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
