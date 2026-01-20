#pragma once
 
#include "FSM.h"
#include "EventDispatcher.h"


class GameManager : public IEventListener
{
public:
	GameManager();
	~GameManager();

	void SetEventDispatcher(EventDispatcher& eventDispatcher) { m_EventDispatcher = eventDispatcher; }

	void Reset();

	void OnEvent(EventType type, const void* data);

	void Initial();

private:

	EventDispatcher& m_EventDispatcher;
};

