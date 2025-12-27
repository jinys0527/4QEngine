#pragma once
#include "Object.h"
#include <windows.h>

class RectTransformComponent;
class UIButtonComponent;
class UISliderComponent;

class UIObject : public Object
{
	friend class UIManager;
	friend class Scene;
	friend class TitleScene;
	friend class InGameUITestScene;
	friend class BossScene;
public:
	UIObject(EventDispatcher& eventDispatcher);
	virtual ~UIObject() = default;

	//void Render(std::vector<UIRenderInfo>& renderInfo);
	//void Render(std::vector<UITextInfo>& textInfo);

	void SetZOrder(int zOrder) { m_ZOrder = zOrder; }
	int GetZOrder() const { return m_ZOrder; }

	bool HitCheck(const POINT& pos);
	
	void SetIsFullScreen(bool isFullScreen) { m_IsFullScreen = isFullScreen; }
	bool IsFullScreen();

	void SetIsVisible(bool isVisible) 
	{
		m_IsVisible = isVisible;

		//// 자식들한테도 재귀 호출해서 동일하게 적용
		//for (auto& child : m_RectTransform->GetChild())
		//{
		//	if (child)
		//	{
		//		auto owner = dynamic_cast<UIObject*>(child->GetOwner());
		//		if (owner)
		//		{
		//			owner->SetIsVisible(isVisible);
		//		}
		//	}
		//}
	}
	bool IsVisible();

	// UI 오브젝트 컴포넌트 추가/삭제 시 플래그 갱신 예시 (UIObject 내부)
	void UpdateInteractableFlags();
	

protected:
	// UI 오브젝트에 컴포넌트 보유 여부 플래그 (UIObject 클래스에 추가)
	bool hasButton = false;
	bool hasSlider = false;

	RectTransformComponent* m_RectTransform;
	int m_ZOrder = 0;
	bool m_IsFullScreen = false;
	bool m_IsVisible = true;
};

