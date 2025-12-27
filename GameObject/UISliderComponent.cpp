#include "UISliderComponent.h"
#include "Object.h"
#include "EventDispatcher.h"
#include "UIUtils.h"

UISliderComponent::UISliderComponent()
{
}

UISliderComponent::~UISliderComponent()
{
	//m_Owner->GetEventDispatcher().RemoveListener(EventType::Pressed, this);
	//m_Owner->GetEventDispatcher().RemoveListener(EventType::Dragged, this);
	//m_Owner->GetEventDispatcher().RemoveListener(EventType::Released, this);
}

void UISliderComponent::Update(float deltaTime)
{

	if (m_IsDirty)
	{
		UpdateFillImage(); //fill 이미지 갱신
		m_IsDirty = false;
	}
}

void UISliderComponent::OnEvent(EventType type, const void* data)
{
	auto mouseData = static_cast<const Events::MouseState*>(data);

	float percentage;
	switch (type)
	{
	case EventType::Pressed:
		if (!IsMouseOverSlider(mouseData->pos))
			return;
		m_IsDragging = true;
		m_CurrentMousePos = mouseData->pos;
		break;

	case EventType::Dragged:
		if (!m_IsDragging)
			return;
		// 슬라이더 위 여부 체크 제거하여 드래그 중 UI 밖으로 마우스 나가도 처리 가능하게 변경
		// if (!IsMouseOverSlider(mouseData->pos))
		//     return;
		m_CurrentMousePos = mouseData->pos;
		break;

	case EventType::Released:
		if (!m_IsDragging)
			return;
		// 마찬가지로 UI 밖이어도 드래그 끝낼 수 있도록 체크 제거
		// if (!IsMouseOverSlider(mouseData->pos))
		//     return;

		float width = std::abs(m_MaxX - m_MinX);
		float offset = m_CurrentMousePos.x - min(m_MinX, m_MaxX);
		percentage = std::clamp(offset / width, 0.0f, 1.0f);
		SetValue(percentage);  // OnValueChanged 콜백 호출
		m_IsDragging = false;
		break;
	}
}

void UISliderComponent::Serialize(nlohmann::json& j) const
{
}

void UISliderComponent::Deserialize(const nlohmann::json& j)
{
}

void UISliderComponent::UpdateFillImage()
{
	auto rect = m_OriginalUV;
	float width = rect.right - rect.left;
	float height = rect.bottom - rect.top;

	switch (m_Direction)
	{
	case FillDirection::LeftToRight:
		rect.right = rect.left + width * m_Value;
		break;
	case FillDirection::RightToLeft:
		rect.left = rect.right - width * m_Value;
		break;
	case FillDirection::BottomToTop:
		rect.top = rect.bottom - height * m_Value;
		break;
	case FillDirection::TopToBottom:
		rect.bottom = rect.top + height * m_Value;
		break;
	}
	m_Fill->GetComponent<UIImageComponent>()->SetUV(rect);
}

void UISliderComponent::Start()
{
// 	m_Owner->GetEventDispatcher().AddListener(EventType::Pressed, this);
// 	m_Owner->GetEventDispatcher().AddListener(EventType::Dragged, this);
// 	m_Owner->GetEventDispatcher().AddListener(EventType::Released, this);
}

bool UISliderComponent::IsMouseOverSlider(const POINT& mousePos)
{
	auto rectTransform = m_Frame->GetComponent<RectTransformComponent>();
	auto size = rectTransform->GetSize();
	auto pos = rectTransform->GetPosition();
	auto pivot = rectTransform->GetPivot();

	return IsPointInUIRect(pos, size, pivot, mousePos);
}
