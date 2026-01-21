//editor
#include "pch.h"
#include "UIManager.h"
#include "Event.h"
#include "UIButtonComponent.h"
#include "UISliderComponent.h"
#include <algorithm>

UIManager::~UIManager()
{
	Reset();
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
	}
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

void UIManager::DeSerializeSceneUI(const std::string& sceneName, const nlohmann::json& data)
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
		uiObject->DeSerialize(entry);
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