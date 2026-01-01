#pragma once
#include "IEventListener.h"
#include <unordered_map>
#include <vector>

class EventDispatcher
{
public:
	void AddListener(EventType type, IEventListener* listener);
	void RemoveListener(EventType type, IEventListener* listener);
	void Dispatch(EventType type, const void* data);

private :
	std::unordered_map<EventType, std::vector<IEventListener*>> m_Listeners;
};

