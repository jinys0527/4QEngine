//editor
#include "pch.h"
#include "UIManager.h"
#include "Event.h"
#include "UIButtonComponent.h"
#include "HorizontalBox.h"
#include "UIProgressBarComponent.h"
#include "UISliderComponent.h"
#include "UITextComponent.h"
#include "Canvas.h"
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

bool UIManager::RegisterSliderOnValueChanged(const std::string& sceneName, const std::string& objectName, std::function<void(float)> callback)
{
	auto callbackCopy = std::move(callback);
	return ApplyToComponent<UISliderComponent>(sceneName, objectName, [callback = std::move(callbackCopy)](UISliderComponent& slider) mutable
		{
			slider.SetOnValueChanged(std::move(callback));
		});
}

bool UIManager::ClearSliderOnValueChanged(const std::string& sceneName, const std::string& objectName)
{
	return ApplyToComponent<UISliderComponent>(sceneName, objectName, [](UISliderComponent& slider)
		{
			slider.SetOnValueChanged(nullptr);
		});
}

bool UIManager::BindButtonToggleVisibility(const std::string& sceneName, const std::string& buttonName, const std::string& targetName)
{
	if (buttonName.empty() || targetName.empty())
		return false;

	m_ButtonBindingsByScene[sceneName][buttonName] = targetName;
	return RegisterButtonOnClicked(sceneName, buttonName, [this, sceneName, targetName]()
		{
			auto target = FindUIObject(sceneName, targetName);
			if (!target)
			{
				return;
			}
			target->SetIsVisible(!target->IsVisible());
		});
}

bool UIManager::ClearButtonBinding(const std::string& sceneName, const std::string& buttonName)
{
	auto itScene = m_ButtonBindingsByScene.find(sceneName);
	if (itScene != m_ButtonBindingsByScene.end())
	{
		itScene->second.erase(buttonName);
	}
	return ClearButtonOnClicked(sceneName, buttonName);
}

bool UIManager::BindSliderToProgress(const std::string& sceneName, const std::string& sliderName, const std::string& targetName)
{
	if (sliderName.empty() || targetName.empty())
		return false;

	m_SliderBindingsByScene[sceneName][sliderName] = targetName;
	return RegisterSliderOnValueChanged(sceneName, sliderName, [this, sceneName, sliderName, targetName](float)
		{
			auto sliderObject = FindUIObject(sceneName, sliderName);
			auto target = FindUIObject(sceneName, targetName);
			if (!sliderObject || !target)
				return;

			auto* slider = sliderObject->GetComponent<UISliderComponent>();
			auto* progress = target->GetComponent<UIProgressBarComponent>();
			if (!slider || !progress)
				return;

			progress->SetPercent(slider->GetNormalizedValue());
		});
}

bool UIManager::ClearSliderBinding(const std::string& sceneName, const std::string& sliderName)
{
	auto itScene = m_SliderBindingsByScene.find(sceneName);
	if (itScene != m_SliderBindingsByScene.end())
	{
		itScene->second.erase(sliderName);
	}

	return ClearSliderOnValueChanged(sceneName, sliderName);
}

const std::unordered_map<std::string, std::string>& UIManager::GetButtonBindings(const std::string& sceneName) const
{
	static const std::unordered_map<std::string, std::string> empty;
	auto it = m_ButtonBindingsByScene.find(sceneName);
	if (it == m_ButtonBindingsByScene.end())
		return empty;

	return it->second;
}

const std::unordered_map<std::string, std::string>& UIManager::GetSliderBindings(const std::string& sceneName) const
{
	static const std::unordered_map<std::string, std::string> empty;
	auto it = m_SliderBindingsByScene.find(sceneName);
	if (it == m_SliderBindingsByScene.end())
		return empty;

	return it->second;
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
			updatedSlot.child     = child.get();
			updatedSlot.childName = child->GetName();
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
		UIObject* child = slot.child;
		if (!child && !slot.childName.empty())
		{
			auto resolved = FindUIObject(sceneName, slot.childName);
			child = resolved ? resolved.get() : nullptr;
		}

		if (child)
		{
			child->ClearParentName();
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
		if (!child && !slots[i].childName.empty())
		{
			auto resolved = FindUIObject(sceneName, slots[i].childName);
			child = resolved ? resolved.get() : nullptr;
		}
		if (!child)
		{
			continue;
		}
		child->SetBounds(arranged[i]);
	}

	return true;
}

bool UIManager::RegisterCanvasSlot(const std::string& sceneName, const std::string& canvasName, const std::string& childName, const CanvasSlot& slot)
{
	auto child = FindUIObject(sceneName, childName);
	if (!child)
		return false;

	const bool applied = ApplyToComponent<Canvas>(sceneName, canvasName, [&](Canvas& canvas)
		{
			CanvasSlot updatedSlot = slot;
			updatedSlot.child = child.get();
			updatedSlot.childName = child->GetName();
			canvas.AddSlot(slot);
		});

	if (!applied)
		return false;

	child->SetParentName(canvasName);
	ApplyCanvasLayout(sceneName, canvasName);
	return true;
}

bool UIManager::RemoveCanvasSlot(const std::string& sceneName, const std::string& canvasName, const std::string& childName)
{
	auto child = FindUIObject(sceneName, childName);
	if (!child)
		return false;

	auto parent = FindUIObject(sceneName, canvasName);
	if (!parent)
		return false;

	auto* canvas = parent->GetComponent<Canvas>();
	if (!canvas)
		return false;

	if (!canvas->RemoveSlotByChild(child.get()))
		return false;

	child->ClearParentName();
	ApplyCanvasLayout(sceneName, canvasName);
	return true;
}

bool UIManager::ClearCanvasSlots(const std::string& sceneName, const std::string& canvasName)
{
	auto parent = FindUIObject(sceneName, canvasName);
	if (!parent)
		return false;

	auto* canvas = parent->GetComponent<Canvas>();
	if (!canvas)
		return false;

	for (auto& slot : canvas->GetSlots())
	{
		UIObject* child = slot.child;
		if (!child && !slot.childName.empty())
		{
			auto resolved = FindUIObject(sceneName, slot.childName);
			child = resolved ? resolved.get() : nullptr;
		}
		if (child)
		{
			child->ClearParentName();
		}
	}
	canvas->ClearSlots();
	return true;
}

bool UIManager::ApplyCanvasLayout(const std::string& sceneName, const std::string& canvasName)
{
	auto parent = FindUIObject(sceneName, canvasName);
	if (!parent)
		return false;

	auto canvas = parent->GetComponent<Canvas>();
	if (!canvas)
		return false;

	for (const auto& slot : canvas->GetSlots())
	{
		UIObject* child = slot.child;
		if (!child && !slot.childName.empty())
		{
			auto resolved = FindUIObject(sceneName, slot.childName);
			child = resolved ? resolved.get() : nullptr;
		}
		if (!child)
		{
			continue;
		}
		child->SetBounds(slot.rect);
	}

	return true;
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

		RenderData::UIElement element{};
		const auto& bounds = uiObject->GetBounds();
		element.position = { bounds.x, bounds.y };
		element.size	 = { bounds.width, bounds.height };
		element.rotation = uiObject->GetRotationDegrees();
		element.zOrder   = uiObject->GetZOrder();
		if (auto* base = uiObject->GetComponent<UIComponent>())
		{
			element.opacity = base->GetOpacity();
		}
		frameData.uiElements.push_back(element);


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
	auto it = m_UIObjects.find(sceneName);
	nlohmann::json objects = nlohmann::json::array();
	if (it != m_UIObjects.end())
	{
		for (const auto& [name, uiObject] : it->second)
		{
			if (!uiObject)
			{
				continue;
			}
			nlohmann::json entry;
			uiObject->Serialize(entry);
			objects.push_back(entry);
		}
	}

	nlohmann::json bindings = nlohmann::json::object();
	auto itButtons = m_ButtonBindingsByScene.find(sceneName);
	if (itButtons != m_ButtonBindingsByScene.end())
	{
		for (const auto& [source, target] : itButtons->second)
		{
			bindings["buttons"][source] = target;
		}
	}
	auto itSliders = m_SliderBindingsByScene.find(sceneName);
	if (itSliders != m_SliderBindingsByScene.end())
	{
		for (const auto& [source, target] : itSliders->second)
		{
			bindings["sliders"][source] = target;
		}
	}

	out = nlohmann::json::object();
	out["objects"] = objects;
	out["bindings"] = bindings;
}



void UIManager::DeserializeSceneUI(const std::string& sceneName, const nlohmann::json& data)
{
	if (!m_EventDispatcher)
	{
		return;
	}

	auto& uiMap = m_UIObjects[sceneName];
	uiMap.clear();
	m_ButtonBindingsByScene.erase(sceneName);
	m_SliderBindingsByScene.erase(sceneName);

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
		if (data.contains("bindings") && data["bindings"].is_object())
		{
			const auto& bindings = data["bindings"];
			if (bindings.contains("buttons") && bindings["buttons"].is_object())
			{
				for (auto itBind = bindings["buttons"].begin(); itBind != bindings["buttons"].end(); ++itBind)
				{
					m_ButtonBindingsByScene[sceneName][itBind.key()] = itBind.value().get<std::string>();
				}
			}
			if (bindings.contains("sliders") && bindings["sliders"].is_object())
			{
				for (auto itBind = bindings["sliders"].begin(); itBind != bindings["sliders"].end(); ++itBind)
				{
					m_SliderBindingsByScene[sceneName][itBind.key()] = itBind.value().get<std::string>();
				}
			}
		}
	}

	for (const auto& entry : objects)
	{
		auto uiObject = std::make_shared<UIObject>(*m_EventDispatcher);
		uiObject->Deserialize(entry);
		uiObject->UpdateInteractableFlags();
		uiMap[uiObject->GetName()] = uiObject;
	}

	for (auto& [name, uiObject] : uiMap)
	{
		if (!uiObject)
		{
			continue;
		}

		if (auto* horizontal = uiObject->GetComponent<HorizontalBox>())
		{
			for (auto& slot : horizontal->GetSlotsRef())
			{
				if (!slot.child && !slot.childName.empty())
				{
					auto itChild = uiMap.find(slot.childName);
					if (itChild != uiMap.end())
					{
						slot.child = itChild->second.get();
					}
				}
				if (slot.child)
				{
					slot.child->SetParentName(uiObject->GetName());
				}
			}
			ApplyHorizontalLayout(sceneName, name);
		}

		if (auto* canvas = uiObject->GetComponent<Canvas>())
		{
			for (auto& slot : canvas->GetSlotsRef())
			{
				if (!slot.child && !slot.childName.empty())
				{
					auto itChild = uiMap.find(slot.childName);
					if (itChild != uiMap.end())
					{
						slot.child = itChild->second.get();
					}
				}
				if (slot.child)
				{
					slot.child->SetParentName(uiObject->GetName());
				}
			}
			ApplyCanvasLayout(sceneName, name);
		}
	}

	if (auto itButtons = m_ButtonBindingsByScene.find(sceneName); itButtons != m_ButtonBindingsByScene.end())
	{
		for (const auto& [source, target] : itButtons->second)
		{
			BindButtonToggleVisibility(sceneName, source, target);
		}
	}
	if (auto itSliders = m_SliderBindingsByScene.find(sceneName); itSliders != m_SliderBindingsByScene.end())
	{
		for (const auto& [source, target] : itSliders->second)
		{
			BindSliderToProgress(sceneName, source, target);
		}
	}

	UpdateSortedUI(uiMap);
}

void UIManager::DispatchToTopUI(EventType type, const void* data)
{
}

void UIManager::RemoveBindingsForObject(const std::string& sceneName, const std::string& objectName)
{
	auto itButtons = m_ButtonBindingsByScene.find(sceneName);
	if (itButtons != m_ButtonBindingsByScene.end())
	{
		itButtons->second.erase(objectName);
		for (auto it = itButtons->second.begin(); it != itButtons->second.end();)
		{
			if (it->second == objectName)
			{
				it = itButtons->second.erase(it);
				continue;
			}
			++it;
		}
	}

	auto itSliders = m_SliderBindingsByScene.find(sceneName);
	if (itSliders != m_SliderBindingsByScene.end())
	{
		itSliders->second.erase(objectName);
		for (auto it = itSliders->second.begin(); it != itSliders->second.end();)
		{
			if (it->second == objectName)
			{
				it = itSliders->second.erase(it);
				continue;
			}
			++it;
		}
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
	m_UIObjects.clear();
	m_ActiveUI = nullptr;
}