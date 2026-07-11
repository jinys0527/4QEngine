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
#include <cctype>
#include <unordered_set>

namespace
{
	int ResolveVendingHoverIndex(const std::string& objectName)
	{
		auto resolveByPrefix = [&](const std::string& prefix)
			{
				if (objectName.rfind(prefix, 0) != 0)
				{
					return 0;
				}

				const std::string suffix = objectName.substr(prefix.size());
				if (suffix.empty())
				{
					return 0;
				}

				for (char ch : suffix)
				{
					if (!std::isdigit(static_cast<unsigned char>(ch)))
					{
						return 0;
					}
				}

				const int parsed = std::stoi(suffix);
				return (parsed >= 1 && parsed <= 6) ? parsed : 0;
			};

		for (const std::string prefix : { "ItemImage", "ItemIcon" })
		{
			const int index = resolveByPrefix(prefix);
			if (index > 0)
			{
				return index;
			}
		}

		return 0;
	}

	bool IsVendingHoverUiName(const std::string& objectName)
	{
		return ResolveVendingHoverIndex(objectName) > 0;
	}


	void DispatchVendingHoverEvent(UIManager& uiManager, const std::string& sceneName, int vendingIndex, bool isHovered)
	{
		if (vendingIndex <= 0)
		{
			return;
		}

		const std::string suffix = std::to_string(vendingIndex);
		const std::string eventName = std::string(isHovered ? "UI_RequestItemInfoShow_Vending" : "UI_RequestItemInfoHide_Vending") + suffix;
		for (const std::string infoName : { "ItemInfo" + suffix, "VendingSlot" + suffix + "Info" })
		{
			auto infoObject = uiManager.FindUIObject(sceneName, infoName);
			if (!infoObject)
			{
				continue;
			}

	m_EventDispatcher = eventDispatcher;
	m_EventDispatcher->AddListener(EventType::Pressed, this);
	m_EventDispatcher->AddListener(EventType::Hovered, this);
	m_EventDispatcher->AddListener(EventType::Dragged, this);
	m_EventDispatcher->AddListener(EventType::Released, this);
}


void UIManager::Start()
{
	m_EventDispatcher.AddListener(EventType::Pressed, this);
	m_EventDispatcher.AddListener(EventType::Hovered, this);
	m_EventDispatcher.AddListener(EventType::Dragged, this);
	m_EventDispatcher.AddListener(EventType::Released, this);
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

	ApplyLayoutOverrides(it->second, m_ViewportSize, m_ReferenceResolution, m_LastResolutionScale, m_LastResolutionOffset, m_HasResolutionScaleState, m_UseAnchorLayout, m_UseResolutionScale);
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
	ApplyLayoutOverrides(uiMap, m_ViewportSize, m_ReferenceResolution, m_LastResolutionScale, m_LastResolutionOffset, m_HasResolutionScaleState, m_UseAnchorLayout, m_UseResolutionScale);
	UpdateSortedUI(uiMap);

	const auto* rawMouseData = static_cast<const Events::MouseState*>(data);
	Events::MouseState scaledMouseData = rawMouseData ? *rawMouseData : Events::MouseState{};

	if (rawMouseData && m_UseResolutionScale && m_HasResolutionScaleState && m_LastResolutionScale > 0.0f)
	{
		scaledMouseData.pos.x = static_cast<LONG>(scaledMouseData.pos.x * m_LastResolutionScale + m_LastResolutionOffset.width);
		scaledMouseData.pos.y = static_cast<LONG>(scaledMouseData.pos.y * m_LastResolutionScale + m_LastResolutionOffset.height);
	}

	const auto* mouseData = rawMouseData ? &scaledMouseData : nullptr;
	auto sendToHitUIs = [&](EventType eventType, UIObject* skipUi = nullptr, bool requireHit = true) {
		bool handledAny = false;

		for (auto* ui : m_SortedUI)
		{
			if (!ui || ui == skipUi || !ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!(ui->hasButton || ui->hasSlider || ui->hasUIFSM))
				continue;
			if (requireHit && (!mouseData || !ui->HitCheck(mouseData->pos)))
				continue;

			if (SendEventToUI(ui, eventType, mouseData))
			{
				handledAny = true;
			}
		}

		if (handledAny && rawMouseData)
			rawMouseData->handled = true;

		return handledAny;
	};

	if (type == EventType::Pressed)
	{
		m_ActiveUI = nullptr;
		bool handledAny = false;

		for (auto* ui : m_SortedUI)
		{
			if (!ui || !ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!(ui->hasButton || ui->hasSlider || ui->hasUIFSM))
				continue;
			if (!mouseData || !ui->HitCheck(mouseData->pos))
				continue;

			if (SendEventToUI(ui, type, mouseData))
			{
				if (!m_ActiveUI && (ui->hasButton || ui->hasSlider))
					m_ActiveUI = ui;
				if (rawMouseData)
					rawMouseData->handled = true;
				handledAny = true;
			}
		}

		if (handledAny && rawMouseData)
			rawMouseData->handled = true;
	}
	else if (type == EventType::UIDragged || type == EventType::Released)
	{
		bool handled = false;
		if (m_ActiveUI)
		{
			handled = SendEventToUI(m_ActiveUI, type == EventType::UIDragged ? EventType::UIDragged : type, mouseData);
		}

		const bool handledByHit = sendToHitUIs(type == EventType::UIDragged ? EventType::UIDragged : type, m_ActiveUI);
		handled = handled || handledByHit;
		if (handled && rawMouseData)
		{
			rawMouseData->handled = true;
		}
		if (type == EventType::Released)
		{
			m_ActiveUI = nullptr;
		}
	}
	else if (type == EventType::UIDoubleClicked)
	{
		sendToHitUIs(EventType::UIDoubleClicked);
	}
	else if (type == EventType::UIHovered)
	{
		if (mouseData)
		{
			if (mouseData)
			{
				bool hoveredByIndex[7] = {};
				UIObject* firstHoveredUiByIndex[7] = {};
				int hoveredCountByIndex[7] = {};

				for (auto* ui : m_SortedUI)
				{
					if (!ui || !ui->IsVisible())
						continue;
					if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
						continue;

					const int vendingIndex = ResolveVendingHoverIndex(ui->GetName());
					if (vendingIndex <= 0)
						continue;

					if (ui->HitCheck(mouseData->pos))
					{
						hoveredByIndex[vendingIndex] = true;
						++hoveredCountByIndex[vendingIndex];
						if (!firstHoveredUiByIndex[vendingIndex])
						{
							firstHoveredUiByIndex[vendingIndex] = ui;
						}
					}
				}

				for (int vendingIndex = 1; vendingIndex <= 6; ++vendingIndex)
				{
					if (hoveredByIndex[vendingIndex] && !m_VendingInfoVisible[vendingIndex])
					{
						const UIObject* firstHoveredUi = firstHoveredUiByIndex[vendingIndex];
						const UIRect bounds = (firstHoveredUi && firstHoveredUi->HasBounds()) ? firstHoveredUi->GetBounds() : UIRect{};
						const std::string hoveredName = firstHoveredUi ? firstHoveredUi->GetName() : "<none>";
						const std::string hoveredParentName = firstHoveredUi ? firstHoveredUi->GetParentName() : "<none>";

						std::cout
							<< "[VendingHover] Show ItemInfo" << vendingIndex
							<< " scene=" << m_CurrentSceneName
							<< " mouse=(" << mouseData->pos.x << "," << mouseData->pos.y << ")"
							<< " hitObject=" << hoveredName
							<< " parent=" << hoveredParentName
							<< " bounds=(x:" << bounds.x << ",y:" << bounds.y
							<< ",w:" << bounds.width << ",h:" << bounds.height << ")"
							<< " hitCount=" << hoveredCountByIndex[vendingIndex]
							<< std::endl;
					}
					m_VendingInfoVisible[vendingIndex] = hoveredByIndex[vendingIndex];
					DispatchVendingHoverEvent(*this, m_CurrentSceneName, vendingIndex, hoveredByIndex[vendingIndex]);
				}
			}
		}

		sendToHitUIs(EventType::UIHovered, nullptr, false);
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

bool UIManager::SendEventToUI(UIObject* ui, EventType type, const void* data)
{
	bool handled = false;

	if (ui->hasButton)
	{
		auto buttons = ui->GetComponents<UIButtonComponent>();
		for (auto* button : buttons)
		{
			if (!button)
				continue;

			const bool isEnabled = button->GetIsEnabled();

			if (type == EventType::Pressed)
			{
				if (isEnabled)
				{
					button->HandlePressed();
					handled = true;
				}
			}
			else if (type == EventType::Released)
			{
				if (isEnabled)
				{
					button->HandleReleased();
					handled = true;
				}
			}
			else if (type == EventType::UIHovered)
			{
				const auto mouseData = static_cast<const Events::MouseState*>(data);
				const bool isHovered = ui->HitCheck(mouseData->pos);
				if (isEnabled)
				{
					button->HandleHover(isHovered);
					handled = true;
				}
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
					handled = true;
				}
				else if (type == EventType::Released)
				{
					slider->HandleReleased();
					handled = true;
				}
			}
		}

		if (ui->hasUIFSM)
		{
			auto* fsm = ui->GetComponent<UIFSMComponent>();
			if (!fsm)
			{
				return handled;
			}
			if (!fsm->ShouldHandleEvent(type, data))
			{
				return handled;
			}

			fsm->OnEvent(type, data);
			handled = true;
		}
	}

	return handled;
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

	ApplyLayoutOverrides(it->second, m_ViewportSize, m_ReferenceResolution, m_LastResolutionScale, m_LastResolutionOffset, m_HasResolutionScaleState, m_UseAnchorLayout, m_UseResolutionScale);

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
		auto resolveOpacity = [](const std::shared_ptr<UIObject> uiObject)
			{
				return uiObject ? uiObject->GetOpacity() : 1.0f;
			};

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

		auto appendElement = [&](const UIRect& rect, int zOrder, const UIImageComponent* image, float opacity, float progress = 1.0f, float progressDirection = 0.0f, const TextureHandle& maskTexture = TextureHandle::Invalid())
			{
				RenderData::UIElement element{};
				element.position = { rect.x, rect.y };
				element.size = { rect.width, rect.height };
				element.rotation = uiObject->GetRotationDegrees();
				element.zOrder = zOrder;
				element.color = image ? image->GetTintColor() : DirectX::XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
				element.opacity = opacity;
				element.progress = progress;
				element.progressDirection = progressDirection;
				element.maskTextureHandle = maskTexture;
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

		const float opacity = resolveOpacity(uiObject);

		if (auto* progress = progressComponent)
		{
			appendElement(bounds, baseZOrder, nullptr, 1.0f);
			auto& backgroundElement = frameData.uiElements.back();
			applyOverrides(backgroundElement,
				progress->GetBackgroundTextureHandle(),
				progress->GetBackgroundShaderAssetHandle(),
				progress->GetBackgroundVertexShaderHandle(),
				progress->GetBackgroundPixelShaderHandle());

			const float percent = std::clamp(progress->GetPercent(), 0.0f, 1.0f);
			if (percent > 0.0f)
			{
				UIRect fillRect = bounds;
				float progressValue = percent;
				const bool useMaskTexture = progress->GetFillMaskTextureHandle().IsValid();

				if (progress->GetFillMode() == UIProgressFillMode::Rect && !useMaskTexture)
				{
					fillRect = buildFillRect(bounds, percent, progress->GetFillDirection());
				}

				const UIFillDirection fillDirection = progress->GetFillDirection();
				const bool isReverseFill = fillDirection == UIFillDirection::RightToLeft
					|| fillDirection == UIFillDirection::BottomToTop;

				const float progressDirection = static_cast<float>(fillDirection);
				appendElement(fillRect, baseZOrder + 1, nullptr, opacity, progressValue, progressDirection, progress->GetFillMaskTextureHandle());

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
			appendElement(bounds, baseZOrder, nullptr, opacity);
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
				appendElement(fillRect, baseZOrder + 1, nullptr, opacity);
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
				appendElement(handleRect, baseZOrder + 2, nullptr, opacity);
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
			appendElement(bounds, baseZOrder, imageComponent, opacity);
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
			text.color.w = opacity;
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

	nlohmann::json objects = nlohmann::json::array();
	if (data.is_array())
	{
		objects = data;
	}
	else if (data.is_object())
	{
		if (data.contains("objects") && data["objects"].is_array())
		{
			objects = data["objects"];
		}
	}

	for (const auto& entry : objects)
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

