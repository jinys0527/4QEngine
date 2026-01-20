#include "FSMActionRegistry.h"

FSMActionRegistry& FSMActionRegistry::Instance()
{
	static FSMActionRegistry instance;

	return instance;
}

void FSMActionRegistry::RegisterAction(const FSMActionDef& def)
{
	auto it = m_ActionIndex.find(def.id);
	if (it != m_ActionIndex.end())
	{
		m_Actions[it->second] = def;
		return;
	}

	m_ActionIndex.emplace(def.id, m_Actions.size());
	m_Actions.push_back(def);
}

const FSMActionDef* FSMActionRegistry::FindAction(const std::string& id) const
{
	auto it = m_ActionIndex.find(id);
	if (it == m_ActionIndex.end())
	{
		return nullptr;
	}

	return &m_Actions[it->second];
}

const std::vector<FSMActionDef>& FSMActionRegistry::GetActions() const
{
	return m_Actions;
}
