#include "UITextComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(UITextComponent, UIComponent)
REGISTER_PROPERTY(UITextComponent, Text)
REGISTER_PROPERTY(UITextComponent, FontSize)

void UITextComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UITextComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}