#pragma once
#include "UIComponent.h"
#include "UIObject.h"
#include <memory>
#include "Event.h"
#include "UIImageComponent.h"
#include "RectTransformComponent.h"


enum class FillDirection
{
	LeftToRight,
	RightToLeft,
	BottomToTop,
	TopToBottom
};

class UISliderComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UISliderComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	UISliderComponent();
	virtual ~UISliderComponent();

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void SetFrame(std::shared_ptr<UIObject>& frameImage)
	{
		m_Frame = frameImage;
	}

	std::shared_ptr<UIObject> GetFrame()
	{
		return m_Frame;
	}

	void SetFill(std::shared_ptr<UIObject>& fillImage)
	{
		m_Fill = fillImage;
		m_OriginalUV = fillImage->GetComponent<UIImageComponent>()->GetUV();
		auto rectTransform = m_Fill->GetComponent<RectTransformComponent>();
		auto size = rectTransform->GetSize(); // sizeDelta 또는 최종 크기
		auto pos = rectTransform->GetPosition();
		auto pivot = rectTransform->GetPivot(); // 0~1 범위

		float left = pos.x;
		float right = left + size.x;

		m_MinX = left;
		m_MaxX = right;
		m_Width = m_MaxX - m_MinX;
	}

	std::shared_ptr<UIObject> GetFill()
	{
		return m_Fill;
	}

	__declspec(noinline)
	void SetValue(float value)
	{
		value = std::clamp(value, 0.0f, 1.0f);

		if (m_Value != value)
		{
			m_Value = value;
			m_IsDirty = true;

			if (m_OnValueChanged)
				m_OnValueChanged(m_Value);
		}
	}

	float GetValue() const
	{
		return m_Value;
	}

	void SetOnValueChanged(const std::function<void(float)>& callback)
	{
		m_OnValueChanged = callback;
	}

	void UpdateFillImage();

	void Start();

	bool IsMouseOverSlider(const POINT& mousePos);
	

private:
	std::shared_ptr<UIObject> m_Frame;
	std::shared_ptr<UIObject> m_Fill;
	D2D1_RECT_F m_OriginalUV;
	float m_Value;
	float m_MinX;
	float m_MaxX;
	float m_Width;
	POINT m_CurrentMousePos;
	bool m_IsDragging = false;
	FillDirection m_Direction = FillDirection::LeftToRight;
	bool m_IsDirty = false;
	std::function<void(float)> m_OnValueChanged;
};

