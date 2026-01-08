#include "UIComponent.h"
#include "ReflectionMacro.h"
REGISTER_COMPONENT(UIComponent);

void UIComponent::Update(float deltaTime)
{
}

void UIComponent::OnEvent(EventType type, const void* data)
{
}

void UIComponent::Serialize(nlohmann::json& j) const
{
}

void UIComponent::Deserialize(const nlohmann::json& j)
{
}

