#include "FSMEventRegistry.h"

FSMEventRegistry& FSMEventRegistry::Instance()
{
	static FSMEventRegistry instance;

	return instance;
}

void FSMEventRegistry::RegisterEvent(const FSMEventDef& def)
{
	auto it = m_EventIndex.find(def.name);
	if (it != m_EventIndex.end())
	{
		m_Events[it->second] = def;
		return;
	}

	m_EventIndex.emplace(def.name, m_Events.size());
	m_Events.push_back(def);
}

const FSMEventDef* FSMEventRegistry::FindEvent(const std::string& name) const
{
	auto it = m_EventIndex.find(name);
	if (it == m_EventIndex.end())
	{
		return nullptr;
	}

	return &m_Events[it->second];
	return nullptr;
}

const std::vector<FSMEventDef>& FSMEventRegistry::GetEvents() const
{
	return m_Events;
}
