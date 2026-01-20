#include "Canvas.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(Canvas, UIComponent)

void Canvas::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void Canvas::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void Canvas::AddSlot(const CanvasSlot& slot)
{
	m_Slots.push_back(slot);
}
