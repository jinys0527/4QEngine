#include "pch.h"
#include "UIManager.h"
#include "Event.h"
#include "UIButtonComponent.h"
#include "HorizontalBox.h"
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

bool UIManager::RegisterButtonOnClicked(const std::string& sceneName, const std::string& objectName, std::function<void()> callback)
{
	auto callbackCopy = std::move(callback);
	return ApplyToComponent<UIButtonComponent>(sceneName, objectName, [callback = std::move(callbackCopy)](UIButtonComponent& button) mutable
		{
			button.SetOnClicked(std::move(callback));
		});
}

bool UIManager::ClearButtonOnClicked(const std::string& sceneName, const std::string& objectName)
{
	return ApplyToComponent<UIButtonComponent>(sceneName, objectName, [](UIButtonComponent& button)
		{
			button.SetOnClicked(nullptr);
		});
}

bool UIManager::RegisterHorizontalSlot(const std::string& sceneName, const std::string& horizontalName, const std::string& childName, const HorizontalBoxSlot& slot)
{
	auto child = FindUIObject(sceneName, childName);
	if (!child)
	{
		return false;
	}

	const bool applied = ApplyToComponent<HorizontalBox>(sceneName, horizontalName, [&](HorizontalBox& horizontal)
		{
			HorizontalBoxSlot updatedSlot = slot;
			updatedSlot.child = child.get();
			horizontal.AddSlot(updatedSlot);
		});
	if (!applied)
	{
		return false;
	}

	child->SetParentName(horizontalName);
	ApplyHorizontalLayout(sceneName, horizontalName);
	return true;
}

bool UIManager::RemoveHorizontalSlot(const std::string& sceneName, const std::string& horizontalName, const std::string& childName)
{
	auto child = FindUIObject(sceneName, childName);
	if (!child)
	{
		return false;
	}

	auto parent = FindUIObject(sceneName, horizontalName);
	if (!parent)
	{
		return false;
	}

	auto* horizontal = parent->GetComponent<HorizontalBox>();
	if (!horizontal)
	{
		return false;
	}

	if (!horizontal->RemoveSlotByChild(child.get()))
	{
		return false;
	}
	child->ClearParentName();
	ApplyHorizontalLayout(sceneName, horizontalName);
	return true;
}

bool UIManager::ClearHorizontalSlots(const std::string& sceneName, const std::string& horizontalName)
{
	auto parent = FindUIObject(sceneName, horizontalName);
	if (!parent)
	{
		return false;
	}

	auto* horizontal = parent->GetComponent<HorizontalBox>();
	if (!horizontal)
	{
		return false;
	}

	for (auto& slot : horizontal->GetSlots())
	{
		if (slot.child)
		{
			slot.child->ClearParentName();
		}
	}
	horizontal->ClearSlots();
	return true;
}

bool UIManager::ApplyHorizontalLayout(const std::string& sceneName, const std::string& horizontalName)
{
	auto parent = FindUIObject(sceneName, horizontalName);
	if (!parent)
	{
		return false;
	}

	auto* horizontal = parent->GetComponent<HorizontalBox>();
	if (!horizontal)
	{
		return false;
	}

	const UIRect parentBounds = parent->GetBounds();
	const UISize availableSize{ parentBounds.width, parentBounds.height };
	const auto arranged = horizontal->ArrangeChildren(parentBounds.x, parentBounds.y, availableSize);
	const auto& slots = horizontal->GetSlots();
	const size_t count = min(arranged.size(), slots.size());

	for (size_t i = 0; i < count; ++i)
	{
		UIObject* child = slots[i].child;
		if (!child)
		{
			continue;
		}
		child->SetBounds(arranged[i]);
	}

	return true;
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