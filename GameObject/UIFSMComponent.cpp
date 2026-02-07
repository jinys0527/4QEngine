#include "UIFSMComponent.h"
#include "FSMActionRegistry.h"
#include "FSMEventRegistry.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "UIObject.h"
#include "UIComponent.h"
#include "UIButtonComponent.h"
#include "UIProgressBarComponent.h"
#include "UISliderComponent.h"
#include "Event.h"
#include "GameState.h"
#include "Scene.h"
#include "GameManager.h"
#include "GameObject.h"
#include "PlayerDoorFSMComponent.h"
#include "PlayerFSMComponent.h"
#include "PlayerShopFSMComponent.h"
#include <algorithm>
#include <cctype>

namespace
{
	bool GraphHasAction(const FSMGraph& graph, const std::string& actionId)
	{
		for (const auto& state : graph.states)
		{
			for (const auto& action : state.onEnter)
			{
				if (action.id == actionId)
				{
					return true;
				}
			}
			for (const auto& action : state.onExit)
			{
				if (action.id == actionId)
				{
					return true;
				}
			}
		}
		return false;
	}

	std::string ToLower(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		return value;
	}

	GameObject* FindPlayerObject(Scene* scene)
	{
		if (!scene)
		{
			return nullptr;
		}

		for (const auto& [name, object] : scene->GetGameObjects())
		{
			(void)name;
			if (!object)
			{
				continue;
			}

			if (object->GetComponent<PlayerFSMComponent>())
			{
				return object.get();
			}
		}

		return nullptr;
	}

	void DispatchPlayerEvent(Scene* scene, const std::string& eventName)
	{
		if (!scene || eventName.empty())
		{
			return;
		}

		auto* player = FindPlayerObject(scene);
		if (!player)
		{
			return;
		}

		if (auto* fsm = player->GetComponent<PlayerFSMComponent>())
		{
			fsm->DispatchEvent(eventName);
		}
	}

	void DispatchPlayerSubEvent(Scene* scene, const std::string& target, const std::string& eventName)
	{
		if (!scene || target.empty() || eventName.empty())
		{
			return;
		}

		auto* player = FindPlayerObject(scene);
		if (!player)
		{
			return;
		}

		const auto normalized = ToLower(target);

		if (normalized == "shop")
		{
			if (auto* fsm = player->GetComponent<PlayerShopFSMComponent>())
			{
				fsm->DispatchEvent(eventName);
			}
			return;
		}

		if (normalized == "door")
		{
			if (auto* fsm = player->GetComponent<PlayerDoorFSMComponent>())
			{
				fsm->DispatchEvent(eventName);
			}
			return;
		}
	}
}

void RegisterUIFSMDefinitions()
{
	auto& actionRegistry = FSMActionRegistry::Instance();
	actionRegistry.RegisterAction({
		"UI_SetVisible",
		"UI",
		{
			{ "value", "bool", true, false }
		}
		});
	actionRegistry.RegisterAction({
		"UI_Show",
		"UI",
		{}
		});
	actionRegistry.RegisterAction({
		"UI_Hide",
		"UI",
		{}
		});
	actionRegistry.RegisterAction({
		"UI_SetOpacity",
		"UI",
		{
			{ "value", "float", 1.0f, false }
		}
		});

	actionRegistry.RegisterAction({
		"UI_SetButtonEnabled",
		"UI",
		{
			{ "value", "bool", true, false }
		}
		});
	actionRegistry.RegisterAction({
		"UI_SetSliderValue",
		"UI",
		{
			{ "value", "float", 0.0f, false }
		}
		});
	actionRegistry.RegisterAction({
		"UI_SetSliderNormalized",
		"UI",
		{
			{ "value", "float", 0.0f, false }
		}
		});
	actionRegistry.RegisterAction({
		"UI_SetProgressPercent",
		"UI",
		{
			{ "value", "float", 0.0f, false }
		}
		});

	actionRegistry.RegisterAction({
		"UI_CacheBounds",
		"UI",
		{}
		});
	actionRegistry.RegisterAction({
		"UI_RestoreBounds",
		"UI",
		{}
		});
	actionRegistry.RegisterAction({
		"UI_ApplyBoundsOffset",
		"UI",
		{
			{ "x", "float", 0.0f, false },
			{ "y", "float", 0.0f, false }
		}
		});

	actionRegistry.RegisterAction({
		"UI_RequestTurnEnd",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({
		"UI_DispatchPlayerEvent",
		"UI",
		{
			{"event", "string", "", true}
		}
		});

	actionRegistry.RegisterAction({
		"UI_DispatchSubFSMEvent",
		"UI",
		{
			{"target", "string", "", true},
			{"event",  "string", "", true}
		}
		});

	actionRegistry.RegisterAction({
		"UI_RequestShopClose",
		"UI",
		{}
		});

	auto& eventRegistry = FSMEventRegistry::Instance();
	eventRegistry.RegisterEvent({ "UI_Pressed", "UI" });
	eventRegistry.RegisterEvent({ "UI_Hovered", "UI" });
	eventRegistry.RegisterEvent({ "UI_Released", "UI" });
	eventRegistry.RegisterEvent({ "UI_Dragged", "UI" });
	eventRegistry.RegisterEvent({ "UI_Clicked", "UI" });
	eventRegistry.RegisterEvent({ "UI_DoubleClicked", "UI" });
	eventRegistry.RegisterEvent({ "UI_SliderValueChanged", "UI" });
	eventRegistry.RegisterEvent({ "UI_ProgressChanged", "UI" });
	eventRegistry.RegisterEvent({ "Player_TurnStart", "UI" });
	eventRegistry.RegisterEvent({ "Player_TurnEnd", "UI" });
	eventRegistry.RegisterEvent({ "Player_ShopOpen", "UI" });
	eventRegistry.RegisterEvent({ "Player_ShopClose", "UI" });
	eventRegistry.RegisterEvent({ "Player_DoorInteract", "UI" });
	eventRegistry.RegisterEvent({ "Player_DoorCancel", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceRoll", "UI" });
}


REGISTER_COMPONENT_DERIVED(UIFSMComponent, FSMComponent)
REGISTER_PROPERTY(UIFSMComponent, EventCallbacks)
REGISTER_PROPERTY(UIFSMComponent, CallbackActions)

UIFSMComponent::UIFSMComponent()
{
	BindActionHandler("UI_SetVisible", [this](const FSMAction& action)
		{
			auto* ui = GetOwner() ? GetOwner()->GetComponent<UIComponent>() : nullptr;
			if (!ui)
				return;

			const bool visible = action.params.value("value", true);
			ui->SetVisible(visible);
		});

	BindActionHandler("UI_Show", [this](const FSMAction&)
		{
			auto* ui = GetOwner() ? GetOwner()->GetComponent<UIComponent>() : nullptr;
			if (!ui)
				return;

			ui->SetVisible(true);
		});
	BindActionHandler("UI_Hide", [this](const FSMAction&)
		{
			auto* ui = GetOwner() ? GetOwner()->GetComponent<UIComponent>() : nullptr;
			if (!ui)
				return;

			ui->SetVisible(false);
		});


	BindActionHandler("UI_SetOpacity", [this](const FSMAction& action)
		{
			auto* ui = GetOwner() ? GetOwner()->GetComponent<UIComponent>() : nullptr;
			if (!ui)
				return;

			const float opacity = action.params.value("value", 1.0f);
			ui->SetOpacity(opacity);
		});

	BindActionHandler("UI_SetButtonEnabled", [this](const FSMAction& action)
		{
			auto* button = GetOwner() ? GetOwner()->GetComponent<UIButtonComponent>() : nullptr;
			if (!button)
				return;

			const bool enabled = action.params.value("value", true);
			button->SetIsEnabled(enabled);
		});

	BindActionHandler("UI_SetSliderValue", [this](const FSMAction& action)
		{
			auto* slider = GetOwner() ? GetOwner()->GetComponent<UISliderComponent>() : nullptr;
			if (!slider)
				return;

			const float value = action.params.value("value", slider->GetValue());
			slider->SetValue(value);
		});

	BindActionHandler("UI_SetSliderNormalized", [this](const FSMAction& action)
		{
			auto* slider = GetOwner() ? GetOwner()->GetComponent<UISliderComponent>() : nullptr;
			if (!slider)
				return;

			const float value = action.params.value("value", slider->GetNormalizedValue());
			slider->SetNormalizedValue(value);
		});

	BindActionHandler("UI_SetProgressPercent", [this](const FSMAction& action)
		{
			auto* progress = GetOwner() ? GetOwner()->GetComponent<UIProgressBarComponent>() : nullptr;
			if (!progress)
				return;

			const float value = action.params.value("value", progress->GetPercent());
			progress->SetPercent(value);
		});

	BindActionHandler("UI_CacheBounds", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* uiObject = owner ? dynamic_cast<UIObject*>(owner) : nullptr;
			if (!uiObject || !uiObject->HasBounds())
			{
				return;
			}
			m_CachedBounds = uiObject->GetBounds();
		});

	BindActionHandler("UI_RestoreBounds", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* uiObject = owner ? dynamic_cast<UIObject*>(owner) : nullptr;
			if (!uiObject || !m_CachedBounds)
			{
				return;
			}
			uiObject->SetBounds(*m_CachedBounds);
			m_CachedBounds.reset();
		});

	BindActionHandler("UI_ApplyBoundsOffset", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* uiObject = owner ? dynamic_cast<UIObject*>(owner) : nullptr;
			if (!uiObject || !uiObject->HasBounds())
			{
				return;
			}
			UIRect bounds = uiObject->GetBounds();
			bounds.x += action.params.value("x", 0.0f);
			bounds.y += action.params.value("y", 0.0f);
			uiObject->SetBounds(bounds);
		});

	BindActionHandler("UI_RequestTurnEnd", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			auto* gameManager = scene ? scene->GetGameManager() : nullptr;
			if (!gameManager)
			{
				return;
			}

			if (gameManager->GetTurn() != Turn::PlayerTurn)
			{
				UpdateTurnEndButtonState(Turn::EnemyTurn);
				return;
			}

			if (gameManager->GetPhase() == Phase::ExplorationLoop)
			{
				GetEventDispatcher().Dispatch(EventType::ExploreTurnEnded, nullptr);
			}
			else
			{
				GetEventDispatcher().Dispatch(EventType::PlayerTurnEndRequested, nullptr);
			}
		});

	BindActionHandler("UI_DispatchPlayerEvent", [this](const FSMAction& action)
		{
			const std::string eventName = action.params.value("event", "");
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			DispatchPlayerEvent(scene, eventName);
		});

	BindActionHandler("UI_DispatchSubFSMEvent", [this](const FSMAction& action)
		{
			const std::string target    = action.params.value("target", "");
			const std::string eventName = action.params.value("event", "");

			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;

			DispatchPlayerSubEvent(scene, target, eventName);
		});

	BindActionHandler("UI_RequestShopClose", [this](const FSMAction& action)
		{
			GetEventDispatcher().Dispatch(EventType::ShopDone, nullptr);
		});
}

UIFSMComponent::~UIFSMComponent()
{
	if(GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::Pressed))
		GetEventDispatcher().RemoveListener(EventType::Pressed, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::UIHovered))
		GetEventDispatcher().RemoveListener(EventType::UIHovered, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::Released))
		GetEventDispatcher().RemoveListener(EventType::Released, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::UIDragged))
		GetEventDispatcher().RemoveListener(EventType::UIDragged, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::UIDoubleClicked))
		GetEventDispatcher().RemoveListener(EventType::UIDoubleClicked, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::TurnChanged))
		GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDoorInteract))
		GetEventDispatcher().RemoveListener(EventType::PlayerDoorInteract, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDoorCancel))
		GetEventDispatcher().RemoveListener(EventType::PlayerDoorCancel, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerShopOpen))
		GetEventDispatcher().RemoveListener(EventType::PlayerShopOpen, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerShopClose))
		GetEventDispatcher().RemoveListener(EventType::PlayerShopClose, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceRoll))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceRoll, this);
}

void UIFSMComponent::Start()
{
	FSMComponent::Start();

	m_HasTurnEndRequestAction = GraphHasAction(GetGraph(), "UI_RequestTurnEnd");


	GetEventDispatcher().AddListener(EventType::Pressed, this);
	GetEventDispatcher().AddListener(EventType::UIHovered, this);
	GetEventDispatcher().AddListener(EventType::Released, this);
	GetEventDispatcher().AddListener(EventType::UIDragged, this);
	GetEventDispatcher().AddListener(EventType::UIDoubleClicked, this);
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
	GetEventDispatcher().AddListener(EventType::PlayerDoorInteract, this);
	GetEventDispatcher().AddListener(EventType::PlayerDoorCancel, this);
	GetEventDispatcher().AddListener(EventType::PlayerShopOpen, this);
	GetEventDispatcher().AddListener(EventType::PlayerShopClose, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceRoll, this);

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (gameManager)
	{
		UpdateTurnEndButtonState(gameManager->GetTurn());
		const auto turnEvent = gameManager->GetTurn() == Turn::PlayerTurn
			? std::string("Player_TurnStart")
			: std::string("Player_TurnEnd");
		HandleEventByName(turnEvent, nullptr);
	}
}

void UIFSMComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::TurnChanged)
	{
		const auto* payload = static_cast<const Events::TurnChanged*>(data);
		if (payload)
		{
			UpdateTurnEndButtonState(static_cast<Turn>(payload->turn));
		}
	}

	if (type == EventType::Pressed
		|| type == EventType::Released
		|| type == EventType::UIDragged
		|| type == EventType::UIDoubleClicked)
	{
		auto* owner = GetOwner();
		auto* uiObject = owner ? dynamic_cast<UIObject*>(owner) : nullptr;
		if (!uiObject || !uiObject->IsVisible() || !uiObject->HasBounds())
		{
			return;
		}

		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || !uiObject->HitCheck(mouseData->pos))
		{
			return;
		}
	}

	const auto eventName = TranslateEvent(type, data);
	if (!eventName)
	{
		return;
	}

	HandleEventByName(*eventName, data);
}

bool UIFSMComponent::ShouldHandleEvent(EventType type, const void* data)
{
	if (type != EventType::UIHovered
		&& type != EventType::Pressed
		&& type != EventType::Released
		&& type != EventType::UIDragged
		&& type != EventType::UIDoubleClicked)
	{
		return true;
	}

	auto* owner = GetOwner();
	auto* uiObject = owner ? dynamic_cast<UIObject*>(owner) : nullptr;
	if (!uiObject || !uiObject->IsVisible())
	{
		return false;
	}

	const auto* mouseData = static_cast<const Events::MouseState*>(data);
	if (!mouseData || !uiObject->HasBounds())
	{
		return true;
	}

	return uiObject->HitCheck(mouseData->pos);
}

void UIFSMComponent::UpdateTurnEndButtonState(Turn turn)
{
	if (!m_HasTurnEndRequestAction)
	{
		return;
	}

	auto* button = GetOwner() ? GetOwner()->GetComponent<UIButtonComponent>() : nullptr;
	if (!button)
	{
		return;
	}

	button->SetIsEnabled(turn == Turn::PlayerTurn);
}

void UIFSMComponent::RegisterCallback(const std::string& id, Callback callback)
{
	if (id.empty())
	{
		return;
	}
	m_Callbacks[id] = std::move(callback);
}

void UIFSMComponent::RegisterCallback(const std::string& id, LegacyCallback callback)
{
	if (id.empty())
	{
		return;
	}
	m_LegacyCallbacks[id] = std::move(callback);
}

void UIFSMComponent::ClearCallback(const std::string& id)
{
	m_Callbacks.erase(id);
}

void UIFSMComponent::TriggerEventByName(const std::string& eventName, const void* data)
{
	if (eventName.empty())
	{
		return;
	}
	HandleEventByName(eventName, data);
}


std::optional<std::string> UIFSMComponent::TranslateEvent(EventType type, const void* data)
{
	switch (type)
	{
	case EventType::Pressed:
		return std::string("UI_Pressed");
	case EventType::UIHovered:
		return std::string("UI_Hovered");
	case EventType::Released:
		return std::string("UI_Released");
	case EventType::UIDragged:
		return std::string("UI_Dragged");
	case EventType::UIDoubleClicked:
		return std::string("UI_DoubleClicked");
	case EventType::TurnChanged:
	{
		if (!data)
		{
			return std::nullopt;
		}
		const auto* payload = static_cast<const Events::TurnChanged*>(data);
		if (!payload)
		{
			return std::nullopt;
		}
		const auto turn = static_cast<Turn>(payload->turn);
		return turn == Turn::PlayerTurn ? std::string("Player_TurnStart")
			: std::string("Player_TurnEnd");
	}
	case EventType::PlayerDoorInteract:
		return std::string("Player_DoorInteract");
	case EventType::PlayerDoorCancel:
		return std::string("Player_DoorCancel");
	case EventType::PlayerShopOpen:
		return std::string("Player_ShopOpen");
	case EventType::PlayerShopClose:
		return std::string("Player_ShopClose");
	case EventType::PlayerDiceRoll:
		return std::string("Player_DiceRoll");
	default:
		return std::nullopt;
	}
}

std::optional<EventType> UIFSMComponent::EventTypeFromName(const std::string& eventName) const
{
	if (eventName == "UI_Pressed")
	{
		return EventType::Pressed;
	}
	if (eventName == "UI_Hovered")
	{
		return EventType::Hovered;
	}
	if (eventName == "UI_Released")
	{
		return EventType::Released;
	}
	if (eventName == "UI_Dragged")
	{
		return EventType::Dragged;
	}
	return std::nullopt;
}

void UIFSMComponent::HandleEventByName(const std::string& eventName, const void* data)
{
	DispatchEvent(eventName);

	for (const auto& entry : m_EventCallbacks)
	{
		if (entry.eventName != eventName)
		{
			continue;
		}

		auto it = m_Callbacks.find(entry.callbackId);
		if (it != m_Callbacks.end())
		{
			it->second(eventName, data);
			continue;
		}

		for (const auto& actionEntry : m_CallbackActions)
		{
			if (actionEntry.callbackId != entry.callbackId)
			{
				continue;
			}
			for (const auto& action : actionEntry.actions)
			{
				HandleAction(action);
			}
			break;
		}
	}
}
