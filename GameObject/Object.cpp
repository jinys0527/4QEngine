#include "Object.h"

Object::Object(EventDispatcher& eventDispatcher) : m_EventDispatcher(eventDispatcher)
{
}

std::vector<std::string> Object::GetComponentTypeNames() const
{
	std::vector<std::string> names;
	names.reserve(m_Components.size());
	for (const auto& entry : m_Components)
	{
		names.push_back(entry.first);
	}
	return names;
}

void Object::Update(float deltaTime)
{
	for (auto it = m_Components.begin(); it != m_Components.end(); it++)
	{
		for (auto& comp : it->second)
		{
			if (comp->GetIsActive())
			{
				comp->Update(deltaTime);
			}
		}
	}
}

void Object::FixedUpdate()
{

}

void Object::SendMessages(const myCore::MessageID msg, void* data /*= nullptr*/)
{
	for (auto it = m_Components.begin(); it != m_Components.end(); it++)
	{
		for (auto& comp : it->second)
		{
			comp->HandleMessage(msg, data);
		}
	}
}

void Object::SendEvent(const std::string& evt)
{
	for (auto it = m_Components.begin(); it != m_Components.end(); it++)
	{
		for (auto& comp : it->second)
		{
			//comp->OnEvent(evt);
		}
	}
}
