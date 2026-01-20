#include "UIProgressBarComponent.h"
#include "ReflectionMacro.h"
#include <algorithm>

REGISTER_COMPONENT_DERIVED(UIProgressBarComponent, UIComponent)
REGISTER_PROPERTY(UIProgressBarComponent, Percent)

void UIProgressBarComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UIProgressBarComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void UIProgressBarComponent::SetPercent(const float& percent)
{
	m_Percent = std::clamp(percent, 0.0f, 1.0f);
}
