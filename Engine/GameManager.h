#pragma once

#include "FSM.h"
#include "EventDispatcher.h"
#include "Event.h"

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

private:

	EventDispatcher* m_EventDispatcher = nullptr;
	Turn m_Turn;
	BattleCheck m_BattleCheck;
	Phase m_Phase; 

};