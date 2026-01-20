#pragma once
#include "UIComponent.h"
#include <functional>

class UIButtonComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIButtonComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetIsEnabled(const bool& enabled) { m_IsEnabled = enabled; }
	const bool& GetIsEnabled() const { return m_IsEnabled; }

	const bool& GetIsPressed() const { return m_IsPressed; }
	const bool& GetIsHovered() const { return m_IsHovered; }

	void SetOnClicked(std::function<void()> callback)
	{
		m_OnClicked = std::move(callback);
	}

	void HandlePressed();
	void HandleReleased();
	void HandleHover(bool isHovered);

private:
	bool m_IsEnabled = true;
	bool m_IsPressed = false;
	bool m_IsHovered = false;
	std::function<void()> m_OnClicked;
};

