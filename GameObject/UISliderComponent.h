#pragma once
#include "UIComponent.h"
#include <functional>

class UISliderComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UISliderComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetRange(const float& minValue, const float& maxValue);
	void SetMinValue(const float& minValue) { SetRange(minValue, m_MaxValue); }
	void SetMaxValue(const float& maxValue) { SetRange(m_MinValue, maxValue); }
	void SetValue(const float& value);
	const float& GetValue() const { return m_Value; }
	const float& GetMinValue() const { return m_MinValue; }
	const float& GetMaxValue() const { return m_MaxValue; }

	void SetNormalizedValue(float normalizedValue);
	float GetNormalizedValue() const;

	const bool& GetIsDragging() const { return m_IsDragging; }

	void SetOnValueChanged(std::function<void(float)> callback)
	{
		m_OnValueChanged = std::move(callback);
	}

	void HandleDrag(float normalizedValue);
	void HandleReleased();

private:
	float m_MinValue  = 0.0f;
	float m_MaxValue  = 1.0f;
	float m_Value     = 0.0f;
	bool m_IsDragging = false;
	std::function<void(float)> m_OnValueChanged;
};

