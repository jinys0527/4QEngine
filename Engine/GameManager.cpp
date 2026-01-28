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

void GameManager::Reset()
{
	TurnReset();
}

void GameManager::OnEvent(EventType type, const void* data)
{

}

void GameManager::TurnReset()
{
	m_Turn = Turn::PlayerTurn;
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
