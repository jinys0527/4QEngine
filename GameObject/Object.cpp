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

Component* Object::GetComponentByTypeName(const std::string& typeName, int index) const
{
	auto it = m_Components.find(typeName);
	if (it == m_Components.end())
	{
		return nullptr;
	}

	const auto& vec = it->second;
	if (index < 0 || index >= static_cast<int>(vec.size()))
	{
		return nullptr;
	}

	return vec[index].get();
}

std::vector<Component*> Object::GetComponentsByTypeName(const std::string& typeName) const
{
	std::vector<Component*> result;
	auto it = m_Components.find(typeName);
	if (it == m_Components.end())
	{
		return result;
	}

	result.reserve(it->second.size());
	for (const auto& comp : it->second)
	{
		result.push_back(comp.get());
	}

	return result;
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
