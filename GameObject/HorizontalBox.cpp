#include "HorizontalBox.h"
#include "ReflectionMacro.h"
#include <algorithm>

REGISTER_COMPONENT_DERIVED(HorizontalBox, UIComponent)

void HorizontalBox::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void HorizontalBox::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void HorizontalBox::AddSlot(const HorizontalBoxSlot& slot)
{
	m_Slots.push_back(slot);
}

bool HorizontalBox::RemoveSlotByChild(const UIObject* child)
{
	const auto endIt = std::remove_if(m_Slots.begin(), m_Slots.end(), [&](const HorizontalBoxSlot& slot)
		{
			return slot.child == child;
		});

	if (endIt == m_Slots.end())
	{
		return false;
	}

	m_Slots.erase(endIt, m_Slots.end());
	return true;
}

void HorizontalBox::ClearSlots()
{
	m_Slots.clear();
}

std::vector<UIRect> HorizontalBox::ArrangeChildren(float startX, float startY, const UISize& availableSize) const
{
	std::vector<UIRect> arranged;
	arranged.reserve(m_Slots.size());

	float totalFixedWidth  = 0.0f;
	float totalFillWeight = 0.0f;

	for (const auto& slot : m_Slots)
	{
		if (slot.alignment == UIHorizontalAlignment::Fill)
		{
			totalFillWeight += slot.fillWeight;
		}
		else
		{
			totalFixedWidth += slot.desiredSize.width + slot.padding * 2.0f;
		}
	}

	float remaining = availableSize.width - totalFixedWidth;
	float cursorX = startX;

	for (const auto& slot : m_Slots)
	{
		float width = slot.desiredSize.width;
		if (slot.alignment == UIHorizontalAlignment::Fill && totalFillWeight > 0.0f)
		{
			width = max(0.0f, remaining * (slot.fillWeight / totalFillWeight));
		}

		const float x = cursorX + slot.padding;
		arranged.push_back(UIRect{ x, startY, width, availableSize.height });

		cursorX += width + slot.padding * 2.0f;
	}

	return arranged;
}
