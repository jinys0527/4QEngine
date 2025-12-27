#include "pch.h"
#include "UIManager.h"
#include "Event.h"
#include <algorithm>
#include "UIButtonComponent.h"
#include "UISliderComponent.h"

UIManager::~UIManager()
{

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
		for (auto& btn : ui->GetComponents<UIButtonComponent>())
			btn->OnEvent(type, data);
	}
	if (ui->hasSlider)
	{
		for (auto& slider : ui->GetComponents<UISliderComponent>())
			slider->OnEvent(type, data);
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
	m_EventDispatcher.RemoveListener(EventType::Pressed, this);
	m_EventDispatcher.RemoveListener(EventType::Hovered, this);
	m_EventDispatcher.RemoveListener(EventType::Dragged, this);
	m_EventDispatcher.RemoveListener(EventType::Released, this);
	m_UIObjects.clear();
	m_ActiveUI = nullptr;
}