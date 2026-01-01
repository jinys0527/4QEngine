#include "EventDispatcher.h"

void EventDispatcher::AddListener(EventType type, IEventListener* listener)
{
	m_Listeners[type].push_back(listener);
}

void EventDispatcher::RemoveListener(EventType type, IEventListener* listener)
{
	auto& v = m_Listeners[type];
	v.erase(std::remove(v.begin(), v.end(), listener), v.end());
}

void EventDispatcher::Dispatch(EventType type, const void* data)
{
	auto it = m_Listeners.find(type);
	if (it != m_Listeners.end())
	{
		for (auto* listener : it->second)
		{
			listener->OnEvent(type, data);
		}
	}
}
