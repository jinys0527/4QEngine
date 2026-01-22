#include "UIComponent.h"
#include "ReflectionMacro.h"
REGISTER_COMPONENT(UIComponent);
REGISTER_PROPERTY(UIComponent, Visible)
REGISTER_PROPERTY(UIComponent, ZOrder)
REGISTER_PROPERTY(UIComponent, Opacity)

void UIComponent::Update(float deltaTime)
{

}

void UIComponent::OnEvent(EventType type, const void* data)
{

}

void UIComponent::Serialize(nlohmann::json& j) const
{
	Component::Serialize(j);
}

void UIComponent::Deserialize(const nlohmann::json& j)
{
	Component::Deserialize(j);
}

