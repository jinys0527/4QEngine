//Client Game
#include "pch.h"
#include "UIManager.h"
#include "Event.h"
#include "UIButtonComponent.h"
#include "UIImageComponent.h"
#include "HorizontalBox.h"
#include "MaterialComponent.h"
#include "UIProgressBarComponent.h"
#include "UITextComponent.h"
#include "UIFSMComponent.h"
#include "UISliderComponent.h"
#include "Border.h"
#include "ScaleBox.h"
#include "SizeBox.h"
#include <algorithm>

namespace
{
	void ApplySizeBoxOverrides(UIObject& uiObject)
	{
		auto* sizeBox = uiObject.GetComponent<SizeBox>();
		if (!sizeBox || !uiObject.HasBounds())
			return;

		UIRect bounds = uiObject.GetBounds();
		const UISize desired = sizeBox->GetDesiredSize(UISize{ bounds.width, bounds.height });
		if (desired.width != bounds.width || desired.height != bounds.height)
		{
			bounds.width = desired.width;
			bounds.height = desired.height;
			uiObject.SetBounds(bounds);
		}
	}

	void ApplyScaleBoxLayout(UIObject& uiObject, const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
	{
		auto* scaleBox = uiObject.GetComponent<ScaleBox>();
		if (!scaleBox || !uiObject.HasBounds())
			return;

		const std::string& parentName = uiObject.GetParentName();
		if (parentName.empty())
			return;

		auto itParent = uiMap.find(parentName);
		if (itParent == uiMap.end() || !itParent->second || !itParent->second->HasBounds())
			return;

		const UIRect parentBounds = itParent->second->GetBounds();
		UIRect bounds = uiObject.GetBounds();

		const UISize scaled = scaleBox->CalculateScaledSize(
			UISize{ parentBounds.width, parentBounds.height },
			UISize{ bounds.width, bounds.height });
		bounds.width = scaled.width;
		bounds.height = scaled.height;
		bounds.x = parentBounds.x + (parentBounds.width - scaled.width) * 0.5f;
		bounds.y = parentBounds.y + (parentBounds.height - scaled.height) * 0.5f;
		uiObject.SetBounds(bounds);
	}

	// 	void ApplyBorderLayout(UIObject& borderObject, const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
	// 	{
	// 		auto* border = borderObject.GetComponent<Border>();
	// 		if (!border || !borderObject.HasBounds())
	// 			return;
	// 
	// 		const std::string& parentName = borderObject.GetName();
	// 		const UIRect contentBounds = border->GetContentRect(borderObject.GetBounds());
	// 
	// 		for (const auto& [name, child] : uiMap)
	// 		{
	// 			if (!child || !child->HasBounds())
	// 			{
	// 				continue;
	// 			}
	// 
	// 			child->SetBounds(contentBounds);
	// 		}
	// 	} // 트리 구조도 아니고 시간 없어서 안쓸것같음

	void ApplyHorizontalBoxLayout(UIObject& uiObject, const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
	{
		auto* horizontalBox = uiObject.GetComponent<HorizontalBox>();
		if (!horizontalBox || !uiObject.HasBounds())
			return;

		auto& slots = horizontalBox->GetSlotsMutable();
		for (auto& slot : slots)
		{
			if (slot.child || slot.childName.empty())
			{
				continue;
			}

			auto itChild = uiMap.find(slot.childName);
			if (itChild != uiMap.end())
			{
				slot.child = itChild->second.get();
			}
		}

		const UIRect parentBounds = uiObject.GetBounds();
		const bool   parentVisible = uiObject.IsVisible();
		const int    parentZOrder = uiObject.GetZOrder();
		const UISize availableSize{ parentBounds.width, parentBounds.height };
		const auto arranged = horizontalBox->ArrangeChildren(parentBounds.x, parentBounds.y, availableSize);
		const size_t count = std::min(arranged.size(), slots.size());
		for (size_t i = 0; i < count; ++i)
		{
			if (slots[i].child)
			{
				slots[i].child->SetBounds(arranged[i]);
				slots[i].child->SetIsVisible(parentVisible);
				slots[i].child->SetZOrder(parentZOrder + static_cast<int>(i) + 1);
			}
		}
	}

	void ApplyLayoutOverrides(const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
	{
		for (const auto& [name, uiObject] : uiMap)
		{
			if (uiObject)
			{
				ApplySizeBoxOverrides(*uiObject);
			}
		}
		
		for (const auto& [name, uiObject] : uiMap)
		{
			if (uiObject)
			{
				ApplyScaleBoxLayout(*uiObject, uiMap);
			}
		}

		for (const auto& [name, uiObject] : uiMap)
		{
			if (uiObject)
			{
				ApplyHorizontalBoxLayout(*uiObject, uiMap);
			}
		}
	}
}


UIManager::~UIManager()
{

}


void UIManager::SetEventDispatcher(EventDispatcher* eventDispatcher)
{
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::Pressed))
	{
		m_EventDispatcher->RemoveListener(EventType::Pressed, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::UIHovered))
	{
		m_EventDispatcher->RemoveListener(EventType::UIHovered, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::UIDragged))
	{
		m_EventDispatcher->RemoveListener(EventType::UIDragged, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::UIDoubleClicked))
	{
		m_EventDispatcher->RemoveListener(EventType::UIDoubleClicked, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::Released))
	{
		m_EventDispatcher->RemoveListener(EventType::Released, this);
	}

	m_EventDispatcher = eventDispatcher;
	m_EventDispatcher->AddListener(EventType::Pressed, this);
	m_EventDispatcher->AddListener(EventType::UIHovered, this);
	m_EventDispatcher->AddListener(EventType::UIDragged, this);
	m_EventDispatcher->AddListener(EventType::UIDoubleClicked, this);
	m_EventDispatcher->AddListener(EventType::Released, this);
}


void UIManager::Start()
{

}


bool UIManager::IsFullScreenUIActive() const
{
	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return false;

	const auto& uiMap = it->second;
	for (const auto& pair : uiMap)
	{
		const auto& uiObject = pair.second;
		if (uiObject->IsVisible() && uiObject->IsFullScreen())
			return true;
	}
	return false;
}

void UIManager::Update(float deltaTime)
{
	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return;

	for (auto& pair : it->second)
	{
		pair.second->Update(deltaTime);
	}

	ApplyLayoutOverrides(it->second);
}

std::shared_ptr<UIObject> UIManager::FindUIObject(const std::string& sceneName, const std::string& objectName)
{
	auto it = m_UIObjects.find(sceneName);
	if (it == m_UIObjects.end())
	{
		return nullptr;
	}

	auto itObj = it->second.find(objectName);
	if (itObj == it->second.end())
	{
		return nullptr;
	}

	return itObj->second;
}


void UIManager::OnEvent(EventType type, const void* data)
{
	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return;

	auto& uiMap = it->second;
	ApplyLayoutOverrides(uiMap);
	UpdateSortedUI(uiMap);
	auto mouseData = static_cast<const Events::MouseState*>(data);

	if (type == EventType::Pressed)
	{
		m_ActiveUI = nullptr;
		for (auto* ui : m_SortedUI)
		{
			if (!ui || !ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!(ui->hasButton || ui->hasSlider || ui->hasUIFSM))
				continue;
			if (!ui->HitCheck(mouseData->pos))
				continue;

			m_ActiveUI = ui;
			if (mouseData)
				mouseData->handled = true;
			SendEventToUI(m_ActiveUI, type, data);
			break;
		}
	}
	else if (type == EventType::UIDragged || type == EventType::Released)
	{
		if (m_ActiveUI)
		{
			if (mouseData)
				mouseData->handled = true;
			if (type == EventType::UIDragged)
			{
				SendEventToUI(m_ActiveUI, EventType::UIDragged, data);
			}
			else
			{
				SendEventToUI(m_ActiveUI, type, data);
			}
			if (type == EventType::Released)
				m_ActiveUI = nullptr;
		}
	}
	else if (type == EventType::Released)
	{
		for (auto* ui : m_SortedUI)
		{
			if (!ui || !ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!(ui->hasButton || ui->hasSlider || ui->hasUIFSM))
				continue;
			if (!ui->HitCheck(mouseData->pos))
				continue;

			if (mouseData)
				mouseData->handled = true;
			break;
		}
	}
	else if (type == EventType::UIDoubleClicked)
	{
		for (auto* ui : m_SortedUI)
		{
			if (!ui || !ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!(ui->hasButton || ui->hasSlider || ui->hasUIFSM))
				continue;
			if (!ui->HitCheck(mouseData->pos))
				continue;

			if (mouseData)
				mouseData->handled = true;
			SendEventToUI(ui, EventType::UIDoubleClicked, data);
			break;
		}
	}
	else if (type == EventType::UIHovered)
	{
		// Hover는 모든 UI에 전달, 내부에서 입장/이탈 상태 관리
		bool hitAny = false;
		for (auto* ui : m_SortedUI)
		{
			if (!ui || !ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!ui->hasButton)
				continue;
			if (!hitAny && ui->HitCheck(mouseData->pos))
				hitAny = true;

			SendEventToUI(ui, type, data);
		}

		if (hitAny && mouseData)
			mouseData->handled = true;
	}
}

void UIManager::UpdateSortedUI(const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
{
	m_SortedUI.clear();
	m_SortedUI.reserve(uiMap.size());

	m_FullScreenUIActive = false;
	m_FullScreenZ = -1;

	for (auto& pair : uiMap)
	{
		UIObject* ui = pair.second.get();
		m_SortedUI.push_back(ui);

		if (ui->IsVisible() && ui->IsFullScreen())
		{
			m_FullScreenUIActive = true;
			if (ui->GetZOrder() > m_FullScreenZ)
				m_FullScreenZ = ui->GetZOrder();
		}
	}

	std::sort(m_SortedUI.begin(), m_SortedUI.end(), [](UIObject* a, UIObject* b) {
		return a->GetZOrder() > b->GetZOrder();
		});
}

void UIManager::SendEventToUI(UIObject* ui, EventType type, const void* data)
{
	if (ui->hasButton)
	{
		auto buttons = ui->GetComponents<UIButtonComponent>();
		for (auto* button : buttons)
		{
			if (!button)
				continue;

			if (type == EventType::Pressed)
			{
				button->HandlePressed();
			}
			else if (type == EventType::Released)
			{
				button->HandleReleased();
			}
			else if (type == EventType::UIHovered)
			{
				const auto mouseData = static_cast<const Events::MouseState*>(data);
				const bool isHovered = ui->HitCheck(mouseData->pos);
				button->HandleHover(isHovered);
			}
		}
		if (ui->hasSlider)
		{
			auto sliders = ui->GetComponents<UISliderComponent>();
			for (auto* slider : sliders)
			{
				if (!slider)
					continue;

				if (type == EventType::UIDragged)
				{
					const auto mouseData = static_cast<const Events::MouseState*>(data);
					const auto bounds = ui->GetBounds();
					float normalizedValue = 0.0f;
					const UIFillDirection direction = slider->GetFillDirection();
					const bool isVertical = direction == UIFillDirection::TopToBottom
						|| direction == UIFillDirection::BottomToTop;
					if (isVertical)
					{
						if (bounds.height > 0.0f)
						{
							normalizedValue = (mouseData->pos.y - bounds.y) / bounds.height;
						}
						if (direction == UIFillDirection::BottomToTop)
						{
							normalizedValue = 1.0f - normalizedValue;
						}
					}
					else
					{
						if (bounds.width > 0.0f)
						{
							normalizedValue = (mouseData->pos.x - bounds.x) / bounds.width;
						}
						if (direction == UIFillDirection::RightToLeft)
						{
							normalizedValue = 1.0f - normalizedValue;
						}
					}
					normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
					slider->HandleDrag(normalizedValue);
				}
				else if (type == EventType::Released)
				{
					slider->HandleReleased();
				}
			}
		}
		if (ui->hasUIFSM)
		{
			auto* fsm = ui->GetComponent<UIFSMComponent>();
			if (!fsm)
			{
				return;
			}
			if (type == EventType::UIHovered)
			{
				const auto mouseData = static_cast<const Events::MouseState*>(data);
				if (!ui->HitCheck(mouseData->pos))
				{
					return;
				}
			}
			fsm->OnEvent(type, data);
		}
	}
}

void UIManager::RefreshUIListForCurrentScene()
{
	auto& uiObjects = GetUIObjects();
	auto it = uiObjects.find(m_CurrentSceneName);
	if (it != uiObjects.end())
	{
		UpdateSortedUI(it->second);
		return;
	}

	m_SortedUI.clear();
	m_FullScreenUIActive = false;
	m_FullScreenZ = -1;
	m_ActiveUI = nullptr;
	m_LastHoveredUI = nullptr;
}

void UIManager::BuildUIFrameData(RenderData::FrameData& frameData)
{
	frameData.uiElements.clear();
	frameData.uiTexts.clear();

	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return;

	ApplyLayoutOverrides(it->second);

	for (const auto& [name, uiObject] : it->second)
	{
		if (!uiObject || !uiObject->IsVisible() || !uiObject->HasBounds())
		{
			continue;
		}

		const auto& bounds = uiObject->GetBounds();
		const int baseZOrder = uiObject->GetZOrder();
		const auto* imageComponent = uiObject->GetComponent<UIImageComponent>();
		auto* progressComponent = uiObject->GetComponent<UIProgressBarComponent>();
		auto* sliderComponent = uiObject->GetComponent<UISliderComponent>();
		auto* buttonComponent = uiObject->GetComponent<UIButtonComponent>();
		auto* textComponent = uiObject->GetComponent<UITextComponent>();
		const bool hasVisualElement = imageComponent || progressComponent || sliderComponent || buttonComponent;
		if (!hasVisualElement && !textComponent)
		{
			continue;
		}

		auto uiComp = uiObject->GetComponent<UIComponent>();
		const float opacity = uiComp ? uiComp->GetOpacity() : 1.0f;

		auto applyOverrides = [&](RenderData::UIElement& element,
			const TextureHandle& texture,
			const ShaderAssetHandle& shaderAsset,
			const VertexShaderHandle& vertexShader,
			const PixelShaderHandle& pixelShader
			)
			{
				const bool hasOverrides = texture.IsValid() || shaderAsset.IsValid()
					|| vertexShader.IsValid() || pixelShader.IsValid();
				if (!hasOverrides)
				{
					return;
				}

				element.useMaterialOverrides = true;
				element.materialOverrides.textureHandle = texture;
				element.materialOverrides.shaderAsset = shaderAsset;
				element.materialOverrides.vertexShader = vertexShader;
				element.materialOverrides.pixelShader = pixelShader;
			};

		auto applyImageOverrides = [&](RenderData::UIElement& element, const UIImageComponent* image)
			{
				if (!image)
					return;

				applyOverrides(element,
					image->GetTextureHandle(),
					image->GetShaderAssetHandle(),
					image->GetVertexShaderHandle(),
					image->GetPixelShaderHandle());
			};

		auto appendElement = [&](const UIRect& rect, int zOrder, const UIImageComponent* image)
			{
				RenderData::UIElement element{};
				element.position = { rect.x, rect.y };
				element.size = { rect.width, rect.height };
				element.rotation = uiObject->GetRotationDegrees();
				element.zOrder = zOrder;
				element.color = { 1.0f, 1.0f, 1.0f, 1.0f };
				element.opacity = opacity;
				applyImageOverrides(element, image);
				frameData.uiElements.push_back(element);
			};

		auto buildFillRect = [](const UIRect& rect, float ratio, UIFillDirection direction)
			{
				UIRect fill = rect;
				switch (direction)
				{
				case UIFillDirection::LeftToRight:
					fill.width = rect.width * ratio;
					break;
				case UIFillDirection::RightToLeft:
					fill.width = rect.width * ratio;
					fill.x = rect.x + rect.width - fill.width;
					break;
				case UIFillDirection::TopToBottom:
					fill.height = rect.height * ratio;
					break;
				case UIFillDirection::BottomToTop:
					fill.height = rect.height * ratio;
					fill.y = rect.y + rect.height - fill.height;
					break;
				default:
					break;
				}
				return fill;
			};

		if (auto* progress = progressComponent)
		{
			appendElement(bounds, baseZOrder, nullptr);
			auto& backgroundElement = frameData.uiElements.back();
			applyOverrides(backgroundElement,
				progress->GetBackgroundTextureHandle(),
				progress->GetBackgroundShaderAssetHandle(),
				progress->GetBackgroundVertexShaderHandle(),
				progress->GetBackgroundPixelShaderHandle());

			const float percent = std::clamp(progress->GetPercent(), 0.0f, 1.0f);
			if (percent > 0.0f)
			{
				UIRect fillRect = buildFillRect(bounds, percent, progress->GetFillDirection());
				appendElement(fillRect, baseZOrder + 1, nullptr);
				auto& fillElement = frameData.uiElements.back();
				applyOverrides(fillElement,
					progress->GetFillTextureHandle(),
					progress->GetFillShaderAssetHandle(),
					progress->GetFillVertexShaderHandle(),
					progress->GetFillPixelShaderHandle());
			}
		}
		else if (auto* slider = sliderComponent)
		{
			appendElement(bounds, baseZOrder, nullptr);
			auto& backgroundElement = frameData.uiElements.back();
			applyOverrides(backgroundElement,
				slider->GetBackgroundTextureHandle(),
				slider->GetBackgroundShaderAssetHandle(),
				slider->GetBackgroundVertexShaderHandle(),
				slider->GetBackgroundPixelShaderHandle());

			const float normalized = std::clamp(slider->GetNormalizedValue(), 0.0f, 1.0f);
			if (normalized > 0.0f)
			{
				UIRect fillRect = buildFillRect(bounds, normalized, slider->GetFillDirection());
				appendElement(fillRect, baseZOrder + 1, nullptr);
				auto& fillElement = frameData.uiElements.back();
				applyOverrides(fillElement,
					slider->GetFillTextureHandle(),
					slider->GetFillShaderAssetHandle(),
					slider->GetFillVertexShaderHandle(),
					slider->GetFillPixelShaderHandle());
			}

			const UIFillDirection fillDirection = slider->GetFillDirection();
			const bool isVertical = fillDirection == UIFillDirection::TopToBottom
				|| fillDirection == UIFillDirection::BottomToTop;

			float handleSize = std::min(bounds.width, bounds.height);
			if (slider->HasHandleSizeOverride())
			{
				handleSize = slider->GetHandleSizeOverride();
			}

			if (handleSize > 0.0f)
			{
				UIRect handleRect = bounds;
				handleRect.width = handleSize;
				handleRect.height = handleSize;
				if (isVertical)
				{
					const float ratio = fillDirection == UIFillDirection::BottomToTop ? 1.0f - normalized : normalized;
					handleRect.x = bounds.x + (bounds.width - handleSize) * 0.5f;
					handleRect.y = bounds.y + bounds.height * ratio - handleSize * 0.5f;
					handleRect.y = std::clamp(handleRect.y, bounds.y, bounds.y + bounds.height - handleSize);
				}
				else
				{
					const float ratio = fillDirection == UIFillDirection::RightToLeft ? 1.0f - normalized : normalized;
					handleRect.x = bounds.x + bounds.width * ratio - handleSize * 0.5f;
					handleRect.x = std::clamp(handleRect.x, bounds.x, bounds.x + bounds.width - handleSize);
					handleRect.y = bounds.y + (bounds.height - handleSize) * 0.5f;
				}
				appendElement(handleRect, baseZOrder + 2, nullptr);
				auto& handleElement = frameData.uiElements.back();
				applyOverrides(handleElement,
					slider->GetHandleTextureHandle(),
					slider->GetHandleShaderAssetHandle(),
					slider->GetHandleVertexShaderHandle(),
					slider->GetHandlePixelShaderHandle());
			}
		}
		else if (hasVisualElement)
		{
			appendElement(bounds, baseZOrder, imageComponent);
			if (auto* button = buttonComponent)
			{
				auto& element = frameData.uiElements.back();

				if (button->HasStyleOverrides())
				{
					applyOverrides(element,
						button->GetCurrentTextureHandle(),
						button->GetShaderAssetHandle(),
						button->GetVertexShaderHandle(),
						button->GetPixelShaderHandle());
				}
				if (button->HasColorOverrides())
				{
					element.color = button->GetCurrentTintColor();
				}
			}
		}

		if (auto* textComp = textComponent)
		{
			RenderData::UITextElement text{};
			text.position = { bounds.x, bounds.y };
			text.color = textComp->GetTextColor();
			text.fontSize = textComp->GetFontSize();
			text.text = textComp->GetText();
			frameData.uiTexts.push_back(std::move(text));
		}
	}

	std::sort(frameData.uiElements.begin(), frameData.uiElements.end(), [](const RenderData::UIElement& a, const RenderData::UIElement& b)
		{
			return a.zOrder < b.zOrder;
		});
}

void UIManager::SerializeSceneUI(const std::string& sceneName, nlohmann::json& out) const
{
	out = nlohmann::json::array();
	auto it = m_UIObjects.find(sceneName);
	if (it == m_UIObjects.end())
	{
		return;
	}

	for (const auto& [name, uiObject] : it->second)
	{
		if (!uiObject)
		{
			continue;
		}
		nlohmann::json entry;
		uiObject->Serialize(entry);
		out.push_back(entry);
	}
}

void UIManager::DeserializeSceneUI(const std::string& sceneName, const nlohmann::json& data)
{
	if (!m_EventDispatcher)
	{
		return;
	}

	auto& uiMap = m_UIObjects[sceneName];
	uiMap.clear();

	if (!data.is_array())
	{
		return;
	}

	for (const auto& entry : data)
	{
		auto uiObject = std::make_shared<UIObject>(*m_EventDispatcher);
		uiObject->Deserialize(entry);
		uiObject->UpdateInteractableFlags();
		uiMap[uiObject->GetName()] = uiObject;
	}
	UpdateSortedUI(uiMap);
}



//void UIManager::Render(std::vector<UIRenderInfo>& uiRenderInfo, std::vector<UITextInfo>& uiTextInfo)
//{
//	auto it = m_UIObjects.find(m_CurrentSceneName);
//	if (it == m_UIObjects.end())
//		return;
//
//	for (auto& pair : it->second)
//	{
//		if (!pair.second->IsVisible())
//			continue;
//
//		pair.second->Render(uiRenderInfo);
//		pair.second->Render(uiTextInfo);
//	}
//}

void UIManager::Reset()
{
	m_UIObjects.clear();
	m_ActiveUI = nullptr;
}