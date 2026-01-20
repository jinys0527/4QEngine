#include "UISliderComponent.h"
#include "ReflectionMacro.h"
#include "UIButtonComponent.h"

REGISTER_COMPONENT_DERIVED(UISliderComponent, UIComponent)
REGISTER_PROPERTY(UISliderComponent, Value)
REGISTER_PROPERTY(UISliderComponent, MinValue)
REGISTER_PROPERTY(UISliderComponent, MaxValue)
REGISTER_PROPERTY_READONLY(UISliderComponent, IsDragging)

void UISliderComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UISliderComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void UISliderComponent::SetRange(const float& minValue, const float& maxValue)
{
	if (minValue > maxValue)
	{
		m_MinValue = maxValue;
		m_MaxValue = minValue;
	}
	else
	{
		m_MinValue = minValue;
		m_MaxValue = maxValue;
	}

	
	SetValue(m_Value);
}

void UISliderComponent::SetValue(const float& value)
{
	m_Value = std::clamp(value, m_MinValue, m_MaxValue);
	if (m_OnValueChanged)
	{
		m_OnValueChanged(m_Value);
	}
}

void UISliderComponent::SetNormalizedValue(float normalizedValue)
{
	normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
	const float value = m_MinValue + (m_MaxValue - m_MinValue) * normalizedValue;
	SetValue(value);
}

float UISliderComponent::GetNormalizedValue() const
{
	if (m_MaxValue <= m_MinValue)
	{
		return 0.0f;
	}
	return (m_Value - m_MinValue) / (m_MaxValue - m_MinValue);
	return 0.0f;
}

void UISliderComponent::HandleDrag(float normalizedValue)
{
	m_IsDragging = true;
	SetNormalizedValue(normalizedValue);
}

void UISliderComponent::HandleReleased()
{
	m_IsDragging = false;
}

