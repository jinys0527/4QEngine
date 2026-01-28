#pragma once

#include "FSM.h"
#include "EventDispatcher.h"
#include "Event.h"
#include "GameState.h"

class GameManager : public IEventListener
{
public:
	GameManager();
	~GameManager();

	void SetEventDispatcher(EventDispatcher& eventDispatcher);
	void ClearEventDispatcher();

	void Reset();

	void OnEvent(EventType type, const void* data);
	void TurnReset();

	void Initial();

	void RequestSceneChange(const std::string& name);

	Turn GetTurn() const { return m_Turn; }
	void SetTurn(Turn turn);

private:
	void RegisterEventListeners();
	void UnregisterEventListeners();
	void DispatchTurnChanged();

private:

	EventDispatcher* m_EventDispatcher = nullptr;
	Turn m_Turn;
	Battle m_BattleCheck;
	Phase m_Phase; 

};