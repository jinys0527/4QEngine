#include "pch.h"
#include <iostream>
#include "GameManager.h"
 
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

void GameManager::RegisterEventListeners()
{
	if (!m_EventDispatcher)
		return;

	m_EventDispatcher->AddListener(EventType::AITurnEndRequested, this);
	m_EventDispatcher->AddListener(EventType::AIMeleeAttackRequested, this);
	m_EventDispatcher->AddListener(EventType::AIRangedAttackRequested, this);
	m_EventDispatcher->AddListener(EventType::PlayerTurnEndRequested, this);
	m_EventDispatcher->AddListener(EventType::EnemyTurnEndRequested, this);

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
