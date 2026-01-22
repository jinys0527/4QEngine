#include "pch.h"
#include "UIManager.h"
#include "Event.h"
#include "UIButtonComponent.h"
#include "HorizontalBox.h"
#include "MaterialComponent.h"
#include "UIProgressBarComponent.h"
#include "UITextComponent.h"
#include "UISliderComponent.h"
#include <algorithm>

UIManager::~UIManager()
{

}

void UIManager::Start()
{

}

void UIManager::SetEventDispatcher(EventDispatcher* eventDispatcher)
{
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::Pressed))
	{
		m_EventDispatcher->RemoveListener(EventType::Pressed, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::Hovered))
	{
		m_EventDispatcher->RemoveListener(EventType::Hovered, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::Dragged))
	{
		m_EventDispatcher->RemoveListener(EventType::Dragged, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->FindListeners(EventType::Released))
	{
		m_EventDispatcher->RemoveListener(EventType::Released, this);
	}

	m_EventDispatcher = eventDispatcher;
	m_EventDispatcher->AddListener(EventType::Pressed, this);
	m_EventDispatcher->AddListener(EventType::Hovered, this);
	m_EventDispatcher->AddListener(EventType::Dragged, this);
	m_EventDispatcher->AddListener(EventType::Released, this);
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
	auto mouseData = static_cast<const Events::MouseState*>(data);

	if (type == EventType::Pressed)
	{
		m_ActiveUI = nullptr;
		for (auto& pair : uiMap)
		{
			auto& ui = pair.second;
			if (!ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!(ui->hasButton || ui->hasSlider))
				continue;
			if (!ui->HitCheck(mouseData->pos))
				continue;

			m_ActiveUI = ui.get();
			SendEventToUI(m_ActiveUI, type, data);
			break;
		}
	}
	else if (type == EventType::Dragged || type == EventType::Released)
	{
		if (m_ActiveUI)
		{
			SendEventToUI(m_ActiveUI, type, data);
			if (type == EventType::Released)
				m_ActiveUI = nullptr;
		}
	}
	else if (type == EventType::Hovered)
	{
		// Hover는 모든 UI에 전달, 내부에서 입장/이탈 상태 관리
		for (auto& pair : uiMap)
		{
			auto& ui = pair.second;
			if (!ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!ui->hasButton)
				continue;

			SendEventToUI(ui.get(), type, data);
		}
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
			else if (type == EventType::Hovered)
			{
				const auto mouseData = static_cast<const Events::MouseState*>(data);
				const bool isHovered = ui->HitCheck(mouseData->pos);
				button->HandleHover(isHovered);
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

			if (type == EventType::Dragged)
			{
				const auto mouseData = static_cast<const Events::MouseState*>(data);
				const auto bounds = ui->GetBounds();
				float normalizedValue = 0.0f;
				if (bounds.width > 0.0f)
				{
					normalizedValue = (mouseData->pos.x - bounds.x) / bounds.width;
				}
				slider->HandleDrag(normalizedValue);
			}
			else if (type == EventType::Released)
			{
				slider->HandleReleased();
			}
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


void UIManager::BuildUIFrameData(RenderData::FrameData& frameData) const
{
	frameData.uiElements.clear();
	frameData.uiTexts.clear();

	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return;

	for (const auto& [name, uiObject] : it->second)
	{
		if (!uiObject || !uiObject->IsVisible() || !uiObject->HasBounds())
		{
			continue;
		}

		
		const auto& bounds   = uiObject->GetBounds();
		const int baseZOrder = uiObject->GetZOrder();
		MaterialHandle baseMaterial = MaterialHandle::Invalid();
		if (auto* material = uiObject->GetComponent<MaterialComponent>())
		{
			baseMaterial = material->GetMaterialHandle();
		}

		auto uiComp = uiObject->GetComponent<UIComponent>();
		const float opacity = uiComp ? uiComp->GetOpacity() : 1.0f;

		auto appendElement = [&](const UIRect& rect, int zOrder, const MaterialHandle& material)
			{
				RenderData::UIElement element{};
				element.position = { rect.x, rect.y };
				element.size = { rect.width, rect.height };
				element.rotation = uiObject->GetRotationDegrees();
				element.zOrder = zOrder;
				element.color = { 1.0f, 1.0f, 1.0f, 1.0f };
				element.opacity = opacity;
				element.material = material;
				frameData.uiElements.push_back(element);
			};

		if (auto* progress = uiObject->GetComponent<UIProgressBarComponent>())
		{
			MaterialHandle backgroundMaterial = progress->GetBackgroundMaterialHandle();
			if (!backgroundMaterial.IsValid())
			{
				backgroundMaterial = baseMaterial;
			}
			MaterialHandle fillMaterial = progress->GetFillMaterialHandle();
			if (!fillMaterial.IsValid())
			{
				fillMaterial = backgroundMaterial;
			}

			appendElement(bounds, baseZOrder, backgroundMaterial);

			const float percent = std::clamp(progress->GetPercent(), 0.0f, 1.0f);
			if (percent > 0.0f)
			{
				UIRect fillRect = bounds;
				fillRect.width = bounds.width * percent;
				appendElement(fillRect, baseZOrder + 1, fillMaterial);
			}
		}
		else
		{
			appendElement(bounds, baseZOrder, baseMaterial);
		}

		if (auto* textComp = uiObject->GetComponent<UITextComponent>())
		{
			RenderData::UITextElement text{};
			text.position = { bounds.x, bounds.y };
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