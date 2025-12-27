#include "Object.h"

Object::Object(EventDispatcher& eventDispatcher) : m_EventDispatcher(eventDispatcher)
{
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
