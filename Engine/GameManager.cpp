#include "pch.h"
#include <iostream>
#include "GameManager.h"


// Timer
// 1. 첫번째
enum class Turn {
	PlayerTurn,
	EnemyTurn,
};

//2. Battle Check
enum class BattleCheck {
	NonBattle, // 비전투
	InBattle,  // 전투 중(진입포함)
};

// 비전투 상태 단위
enum class Phase {
	PlayerMove,
	//---------------------
	ItemPick,
	DoorOpen,
	Attack,
	
};
 
GameManager::GameManager()
{

}

GameManager::~GameManager()
{
}

void GameManager::Reset()
{

}

void GameManager::OnEvent(EventType type, const void* data)
{

}

void GameManager::Initial()
{

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
