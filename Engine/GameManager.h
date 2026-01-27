#pragma once

#include "FSM.h"
#include "EventDispatcher.h"
#include "Event.h"

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
// 추후 수정
enum class Phase {
	PlayerMove,
	//---------------------
	ItemPick,
	DoorOpen,
	Attack,

};






class GameManager : public IEventListener
{
public:
	GameManager();
	~GameManager();

	void SetEventDispatcher(EventDispatcher& eventDispatcher) { m_EventDispatcher = &eventDispatcher; }

	void Reset();

	void OnEvent(EventType type, const void* data);

	void Initial();

	void RequestSceneChange(const std::string& name);

	Turn GetTurn() const { return m_Turn; }
	void SetTurn(Turn turn) { m_Turn = turn; }

private:

	EventDispatcher* m_EventDispatcher = nullptr;
	Turn m_Turn;
	BattleCheck m_BattleCheck;
	Phase m_Phase; 

};