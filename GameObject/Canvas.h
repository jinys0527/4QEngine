#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"
#include <vector>

class UIObject;

struct CanvasSlot
{
	UIObject* child = nullptr;
	UIRect    rect{};
};

class Canvas : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "Canvas";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void AddSlot(const CanvasSlot& slot);
	const std::vector<CanvasSlot>& GetSlots() const { return m_Slots; }

private:
	std::vector<CanvasSlot> m_Slots;
};

