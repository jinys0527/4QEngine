#include "EventDispatcher.h"

void EventDispatcher::AddListener(EventType type, IEventListener* listener)
{
	if (!listener) return;

	auto& vec = m_Listeners[type]; // 여기선 생성 OK(등록이니까)
	if (std::find(vec.begin(), vec.end(), listener) != vec.end())
		return;

	vec.push_back(listener);
}

void EventDispatcher::RemoveListener(EventType type, IEventListener* listener)
{
	if (!listener) return;

	auto it = m_Listeners.find(type);
	if (it == m_Listeners.end())
		return;

	auto& vec = it->second;

	// 모두 제거(중복 등록 방지까지 같이 처리)
	vec.erase(std::remove(vec.begin(), vec.end(), listener), vec.end());

	if (vec.empty())
		m_Listeners.erase(it);
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
