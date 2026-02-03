#include "UIComponent.h"
#include "ReflectionMacro.h"
#include "UIObject.h"
REGISTER_UI_COMPONENT(UIComponent);
REGISTER_PROPERTY(UIComponent, Visible)
REGISTER_PROPERTY(UIComponent, ZOrder)
REGISTER_PROPERTY(UIComponent, Opacity)

void UIComponent::Update(float deltaTime)
{

}

void UIComponent::OnEvent(EventType type, const void* data)
{

}

void UIComponent::SetVisible(const bool& visible) 
{
	m_Visible = visible;
	if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
	{
		owner->SetIsVisibleFromComponent(visible);
	}
}

void UIComponent::SetZOrder(const int& value)
{
	m_ZOrder = value;
	if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
	{
		owner->SetZOrderFromComponent(value);
	}
}

void UIComponent::SetOpacity(const float& value)
{
	m_Opacity = value;
}

void UIComponent::Serialize(nlohmann::json& j) const
{
	Component::Serialize(j);
}

void UIComponent::Deserialize(const nlohmann::json& j)
{
	Component::Deserialize(j);
}

