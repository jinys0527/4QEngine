#pragma once
#include "UIComponent.h"

class UIProgressBarComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIProgressBarComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetPercent(const float& percent);
	const float& GetPercent() const { return m_Percent; }

private:
	float m_Percent = 0.0f;
};

