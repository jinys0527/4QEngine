#include "FSM.h"

void FSM::Update(float deltaTime)
{
	auto it = m_States.find(m_CurrentState);
	if (it != m_States.end() && it->second.OnUpdate)
	{
		it->second.OnUpdate(deltaTime);
	}
}

void FSM::SetInitialState(const StateID& id)
{
	auto it = m_States.find(id);
	if (it == m_States.end())
	{
		return;
	}

	m_CurrentState = id;
	if (it->second.OnEnter)
	{
		it->second.OnEnter();
	}
}

void FSM::Trigger(const std::string& evt)
{
	auto it = m_Transitions.find(m_CurrentState);
	if (it != m_Transitions.end())
	{
		auto it2 = it->second.find(evt);
		if (it2 != it->second.end())
		{
			ChangeState(it2->second);
		}
	}
}

void FSM::ChangeState(const StateID& id)
{
	if (m_CurrentState == id) return;

	if (m_States[m_CurrentState].OnExit)
	{
		m_States[m_CurrentState].OnExit();
	}

	m_CurrentState = id;

	if (m_States[m_CurrentState].OnEnter)
	{
		m_States[m_CurrentState].OnEnter();
	}
}
