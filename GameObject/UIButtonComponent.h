#pragma once
#include "UIComponent.h"
#include <d2d1.h>
#include "functional"
#include "Event.h"
#include "FSM.h"

class UIImageComponent;

class UIButtonComponent : public UIComponent
{
public:
	UIButtonComponent();
	static constexpr const char* StaticTypeName = "UIButtonComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	virtual ~UIButtonComponent();

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	FSM& GetFSM() { return m_FSM; }

	void Start();
private:
	bool IsHovered(POINT mousePos);

	bool m_IsClicked = false;
	bool m_IsHovered = false;
	FSM m_FSM;

	D2D1_COLOR_F m_DefaultColor = D2D1::ColorF(D2D1::ColorF::LightGray);
	D2D1_COLOR_F m_HoveredColor = D2D1::ColorF(D2D1::ColorF::Gray);
	D2D1_COLOR_F m_PressedColor = D2D1::ColorF(D2D1::ColorF::DarkGray);
};

REGISTER_COMPONENT(UIButtonComponent);