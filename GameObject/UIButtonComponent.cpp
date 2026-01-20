#include "UIButtonComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(UIButtonComponent, UIComponent)
REGISTER_PROPERTY(UIButtonComponent, IsEnabled)
REGISTER_PROPERTY_READONLY(UIButtonComponent, IsPressed)
REGISTER_PROPERTY_READONLY(UIButtonComponent, IsHovered)

void UIButtonComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UIButtonComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void UIButtonComponent::HandlePressed()
{
	if (!m_IsEnabled)
		return;
	m_IsPressed = true;
}

void UIButtonComponent::HandleReleased()
{
	if (!m_IsEnabled)
		return;

	if (m_IsPressed && m_OnClicked)
	{
		m_OnClicked();
	}

	m_IsPressed = false;
}


void UIButtonComponent::HandleHover(bool isHovered)
{
	if (!m_IsEnabled)
		return;

	m_IsHovered = isHovered;
}
