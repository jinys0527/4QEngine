#include "ControlComponent.h"
#include "GameObject.h"
#include "Object.h"
#include "Event.h"


ControlComponent::ControlComponent()
{
}

ControlComponent::~ControlComponent()
{
	m_Owner->GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
}


void ControlComponent::Start()
{
	m_Owner->GetEventDispatcher().AddListener(EventType::KeyDown, this);
}

void ControlComponent::Update(float deltaTime)
{
}

void ControlComponent::Serialize(nlohmann::json& j) const
{
}

void ControlComponent::Deserialize(const nlohmann::json& j)
{
}

void ControlComponent::OnEvent(EventType type, const void* data)
{
	if (!m_IsCorrectScene)
		return;
	if (type != EventType::KeyDown)
		return;

	auto keyData = static_cast<const Events::KeyEvent*>(data);
	if (!keyData)
		return;

	// any key 콜백 호출
	if (m_AnyKeyCallback && keyData->key != VK_ESCAPE)
		m_AnyKeyCallback();

	int key = keyData->key;

	auto it = m_KeyDownCallbacks.find(key);
	if (it != m_KeyDownCallbacks.end() )
	{
		it->second();
	}
}

void ControlComponent::RegisterKeyDownCallback(int vkKey, std::function<void()> callback)
{
	m_KeyDownCallbacks[vkKey] = callback;
}

void ControlComponent::UnregisterKeyDownCallback(int vkKey)
{
	m_KeyDownCallbacks.erase(vkKey);
}

void ControlComponent::Enter()
{
	m_IsCorrectScene = true;
}

void ControlComponent::Leave()
{
	m_IsCorrectScene = false;
}
