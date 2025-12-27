#pragma once
 
#include "FSM.h"
#include "EventDispatcher.h"


class GameManager : public IEventListener
{
public:
	GameManager(EventDispatcher& eventDispatcher);
	~GameManager();

	void Reset();

	void OnEvent(EventType type, const void* data);

	void Initial();

private:

	EventDispatcher& m_EventDispatcher;
};

