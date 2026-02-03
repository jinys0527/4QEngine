#include "HorizontalBox.h"
#include "ReflectionMacro.h"
#include "UIObject.h"
#include <algorithm>

REGISTER_UI_COMPONENT(HorizontalBox)
REGISTER_PROPERTY(HorizontalBox, Slots)
REGISTER_PROPERTY(HorizontalBox, Spacing)

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
	HorizontalBoxSlot updatedSlot = slot;
	if (updatedSlot.child && updatedSlot.childName.empty())
	{
		updatedSlot.childName = updatedSlot.child->GetName();
	}
	m_Slots.push_back(updatedSlot);
}

void HorizontalBox::SetSlots(const std::vector<HorizontalBoxSlot>& slots)
{
	m_Slots = slots;
	for (auto& slot : m_Slots)
	{
		if (slot.child && slot.childName.empty())
		{
			slot.childName = slot.child->GetName();
		}
	}
}

bool HorizontalBox::RemoveSlotByChild(const UIObject* child)
{
	const std::string targetName = child ? child->GetName() : "";
	const auto endIt = std::remove_if(m_Slots.begin(), m_Slots.end(), [&](const HorizontalBoxSlot& slot)
		{
			if(slot.child == child)
			{
				return true;
			}
			return !targetName.empty() && slot.childName == targetName;
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
		const float slotPadding = (slot.alignment == UIHorizontalAlignment::Fill) ? 0.0f : slot.padding;
		if (slot.alignment == UIHorizontalAlignment::Fill)
		{
			totalFillWeight += slot.fillWeight;
		}
		else
		{
			totalFixedWidth += slot.desiredSize.width;
		}
		totalFixedWidth += slotPadding * 2.0f;
	}

	const float totalSpacing = m_Slots.size() > 1 ? m_Spacing * static_cast<float>(m_Slots.size() - 1) : 0.0f;
	totalFixedWidth += totalSpacing;
	float remaining = availableSize.width - totalFixedWidth;
	remaining = max(0.0f, remaining);
	float cursorX = startX;

	for (size_t index = 0; index < m_Slots.size(); ++index)
	{
		const auto& slot = m_Slots[index];
		const float slotPadding = (slot.alignment == UIHorizontalAlignment::Fill) ? 0.0f : slot.padding;
		float width = slot.desiredSize.width;
		if (slot.alignment == UIHorizontalAlignment::Fill && totalFillWeight > 0.0f)
		{
			width = max(0.0f, remaining * (slot.fillWeight / totalFillWeight));
		}

		const float x = cursorX + slotPadding;
		arranged.push_back(UIRect{ x, startY, width, availableSize.height });

		cursorX += width + slotPadding * 2.0f;

		if (index + 1 < m_Slots.size())
		{
			cursorX += m_Spacing;
		}
	}

	return arranged;
}
