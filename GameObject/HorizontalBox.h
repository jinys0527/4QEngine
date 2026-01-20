#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"
#include <vector>

class UIObject;

enum class UIHorizontalAlignment
{
	Left,
	Center,
	Right,
	Fill
};

struct HorizontalBoxSlot
{
	UIObject*             child		  = nullptr;
	UISize                desiredSize{};
	float                 padding     = 0.0f;
	float                 fillWeight  = 0.0f;
	UIHorizontalAlignment alignment   = UIHorizontalAlignment::Left;
};

class HorizontalBox : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "HorizontalBox";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void AddSlot(const HorizontalBoxSlot& slot);
	const std::vector<HorizontalBoxSlot>& GetSlots() const { return m_Slots; }

	std::vector<UIRect> ArrangeChildren(float startX, float startY, const UISize& availableSize) const;

private:
	std::vector<HorizontalBoxSlot> m_Slots;
};

