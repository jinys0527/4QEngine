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
#include <iostream>

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
		"UI_RequestDiceDecision",
		"UI",
		{}
		});
	actionRegistry.RegisterAction({
		"UI_RequestDiceStatRoll",
		"UI",
		{}
		});
	actionRegistry.RegisterAction({
		"UI_RequestDiceContinue",
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
		"UI_DispatchUIEvent",
		"UI",
		{
			{"event", "string", "", true}
		}
		});

	actionRegistry.RegisterAction({
		"UI_RequestCloseMenu",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({
		"UI_RequestGoToTitle",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({
		"UI_RequestSceneChange",
		"UI",
		{
			{"scene", "string", "", true}
		}
		});

	actionRegistry.RegisterAction({
		"UI_RequestShopClose",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({
		"UI_RequestDoorCancel",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({
		"UI_RequestPlayerMelee",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({
		"UI_RequestPlayerThrow_1",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({
		"UI_RequestPlayerThrow_2",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({
		"UI_RequestPlayerThrow_3",
		"UI",
		{}
		});

	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Melee", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Melee", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Throw1", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Throw1", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Throw2", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Throw2", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Throw3", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Throw3", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Vending1", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Vending1", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Vending2", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Vending2", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Vending3", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Vending3", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Vending4", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Vending4", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Vending5", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Vending5", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoShow_Vending6", "UI", {} });
	actionRegistry.RegisterAction({ "UI_RequestItemInfoHide_Vending6", "UI", {} });

	auto& eventRegistry = FSMEventRegistry::Instance();
	eventRegistry.RegisterEvent({ "UI_Pressed", "UI" });
	eventRegistry.RegisterEvent({ "UI_Hovered", "UI" });
	eventRegistry.RegisterEvent({ "UI_HoverExit", "UI" });
	eventRegistry.RegisterEvent({ "UI_Released", "UI" });
	eventRegistry.RegisterEvent({ "UI_Dragged", "UI" });
	eventRegistry.RegisterEvent({ "UI_Clicked", "UI" });
	eventRegistry.RegisterEvent({ "UI_DoubleClicked", "UI" });
	eventRegistry.RegisterEvent({ "UI_EscapePressed", "UI" });
	eventRegistry.RegisterEvent({ "UI_CloseRequested", "UI" });
	eventRegistry.RegisterEvent({ "UI_GoToTitleRequested", "UI" });
	eventRegistry.RegisterEvent({ "UI_SliderValueChanged", "UI" });
	eventRegistry.RegisterEvent({ "UI_ProgressChanged", "UI" });
	eventRegistry.RegisterEvent({ "Player_TurnStart", "UI" });
	eventRegistry.RegisterEvent({ "Player_TurnEnd", "UI" });
	eventRegistry.RegisterEvent({ "Player_ShopOpen", "UI" });
	eventRegistry.RegisterEvent({ "Player_ShopClose", "UI" });
	eventRegistry.RegisterEvent({ "UI_VendingOfferUpdated", "UI" });
	eventRegistry.RegisterEvent({ "Shop_MoneyOk", "UI" });
	eventRegistry.RegisterEvent({ "Shop_MoneyFail", "UI" });
	eventRegistry.RegisterEvent({ "Player_DoorInteract", "UI" });
	eventRegistry.RegisterEvent({ "Player_DoorCancel", "UI" });
	eventRegistry.RegisterEvent({ "Player_DoorSuccess", "UI" });
	eventRegistry.RegisterEvent({ "Player_DoorFail", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceRoll", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceUIOpen", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceUIReset", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceRollRequested", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceRollApplied", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceTotalsApplied", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceResultShown", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceUIClose", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceDecisionRequested", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceDecisionFaceRolled", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceDecisionResult", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceInitiativeResolved", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceStatRollRequested", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceTypeDetermined", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceStatResolved", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceAnimationStarted", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceAnimationCompleted", "UI" });
	eventRegistry.RegisterEvent({ "Player_DiceContinueRequested", "UI" });
	eventRegistry.RegisterEvent({ "Player_Melee", "UI" });
	eventRegistry.RegisterEvent({ "Player_Throw_1", "UI" });
	eventRegistry.RegisterEvent({ "Player_Throw_2", "UI" });
	eventRegistry.RegisterEvent({ "Player_Throw_3", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Melee", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Melee", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Throw1", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Throw1", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Throw2", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Throw2", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Throw3", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Throw3", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Vending1", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Vending1", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Vending2", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Vending2", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Vending3", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Vending3", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Vending4", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Vending4", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Vending5", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Vending5", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoShow_Vending6", "UI" });
	eventRegistry.RegisterEvent({ "UI_RequestItemInfoHide_Vending6", "UI" });
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

	BindActionHandler("UI_RequestDiceDecision", [this](const FSMAction&)
		{
			// Backward compatibility: 일부 에디터 FSM은 DecisionReady 전이에
			// Player_DiceRollRequested 를 사용하고 있어 먼저 같이 발행한다.
			std::cout << "[UIFSM] Action UI_RequestDiceDecision -> dispatch PlayerDiceRollRequested, PlayerDiceDecisionRequested" << std::endl;
			GetEventDispatcher().Dispatch(EventType::PlayerDiceRollRequested, nullptr);
			GetEventDispatcher().Dispatch(EventType::PlayerDiceDecisionRequested, nullptr);
		});

	BindActionHandler("UI_RequestDiceStatRoll", [this](const FSMAction&)
		{
			std::cout << "[UIFSM] Action UI_RequestDiceStatRoll -> dispatch PlayerDiceStatRollRequested" << std::endl;
			GetEventDispatcher().Dispatch(EventType::PlayerDiceStatRollRequested, nullptr);
		});

	BindActionHandler("UI_RequestDiceContinue", [this](const FSMAction&)
		{
			std::cout << "[UIFSM] Action UI_RequestDiceContinue -> dispatch PlayerDiceContinueRequested" << std::endl;
			GetEventDispatcher().Dispatch(EventType::PlayerDiceContinueRequested, nullptr);
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
			const std::string target = action.params.value("target", "");
			const std::string eventName = action.params.value("event", "");

			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;

			DispatchPlayerSubEvent(scene, target, eventName);
		});

	BindActionHandler("UI_DispatchUIEvent", [this](const FSMAction& action)
		{
			const std::string eventName = action.params.value("event", "");
			if (!eventName.empty())
			{
				DispatchEvent(eventName);
			}
		});

	BindActionHandler("UI_RequestCloseMenu", [this](const FSMAction&)
		{
			GetEventDispatcher().Dispatch(EventType::UICloseRequested, nullptr);
			DispatchEvent("UI_CloseRequested");
		});

	BindActionHandler("UI_RequestGoToTitle", [this](const FSMAction&)
		{
			Events::SceneChangeRequest request;
			request.name = "Title";
			GetEventDispatcher().Dispatch(EventType::SceneChangeRequested, &request);
			GetEventDispatcher().Dispatch(EventType::UIGoToTitleRequested, nullptr);
			DispatchEvent("UI_GoToTitleRequested");
		});

	BindActionHandler("UI_RequestSceneChange", [this](const FSMAction& action)
		{
			const std::string sceneName = action.params.value("scene", "");
			if (sceneName.empty())
			{
				return;
			}

			Events::SceneChangeRequest request;
			request.name = sceneName;
			GetEventDispatcher().Dispatch(EventType::SceneChangeRequested, &request);
			if (sceneName == "Title")
			{
				GetEventDispatcher().Dispatch(EventType::UIGoToTitleRequested, nullptr);
				DispatchEvent("UI_GoToTitleRequested");
			}
		});

	BindActionHandler("UI_RequestShopClose", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			DispatchPlayerEvent(scene, "Shop_Close");
			DispatchEvent("None");
		});

	BindActionHandler("UI_RequestDoorCancel", [this](const FSMAction& action)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
			DispatchEvent("None");
		});

	BindActionHandler("UI_RequestPlayerMelee", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			DispatchPlayerEvent(scene, "Player_Melee");
		});

	BindActionHandler("UI_RequestPlayerThrow_1", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			DispatchPlayerEvent(scene, "Player_Throw_1");
		});

	BindActionHandler("UI_RequestPlayerThrow_2", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			DispatchPlayerEvent(scene, "Player_Throw_2");
		});

	BindActionHandler("UI_RequestPlayerThrow_3", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			DispatchPlayerEvent(scene, "Player_Throw_3");
		});

	auto bindItemInfoHandler = [this](const std::string& actionId, bool visible)
		{
			BindActionHandler(actionId, [this, visible](const FSMAction&)
				{
					auto* ui = GetOwner() ? GetOwner()->GetComponent<UIComponent>() : nullptr;
					if (!ui)
					{
						return;
					}

					ui->SetVisible(visible);
				});
		};

	bindItemInfoHandler("UI_RequestItemInfoShow_Melee", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Melee", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Throw1", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Throw1", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Throw2", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Throw2", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Throw3", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Throw3", false);
	bindItemInfoHandler("UI_RequestItemInfoHide_Throw3", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Vending1", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Vending1", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Vending2", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Vending2", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Vending3", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Vending3", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Vending4", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Vending4", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Vending5", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Vending5", false);
	bindItemInfoHandler("UI_RequestItemInfoShow_Vending6", true);
	bindItemInfoHandler("UI_RequestItemInfoHide_Vending6", false);
}

UIFSMComponent::~UIFSMComponent()
{
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::Pressed))
		GetEventDispatcher().RemoveListener(EventType::Pressed, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::KeyDown))
		GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::UICloseRequested))
		GetEventDispatcher().RemoveListener(EventType::UICloseRequested, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::UIGoToTitleRequested))
		GetEventDispatcher().RemoveListener(EventType::UIGoToTitleRequested, this);
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
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDoorSuccess))
		GetEventDispatcher().RemoveListener(EventType::PlayerDoorSuccess, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDoorFail))
		GetEventDispatcher().RemoveListener(EventType::PlayerDoorFail, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerShopOpen))
		GetEventDispatcher().RemoveListener(EventType::PlayerShopOpen, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerShopClose))
		GetEventDispatcher().RemoveListener(EventType::PlayerShopClose, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::VendingOfferUpdated))
		GetEventDispatcher().RemoveListener(EventType::VendingOfferUpdated, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::ShopMoneyOk))
		GetEventDispatcher().RemoveListener(EventType::ShopMoneyOk, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::ShopMoneyFail))
		GetEventDispatcher().RemoveListener(EventType::ShopMoneyFail, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceRoll))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceRoll, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceUIOpen))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceUIOpen, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceUIReset))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceUIReset, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceRollRequested))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceRollRequested, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceRollApplied))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceRollApplied, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceTotalsApplied))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceTotalsApplied, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceResultShown))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceResultShown, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceUIClose))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceUIClose, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceDecisionRequested))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceDecisionRequested, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceDecisionResult))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceDecisionResult, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceInitiativeResolved))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceInitiativeResolved, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceStatRollRequested))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceStatRollRequested, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceTypeDetermined))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceTypeDetermined, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceStatResolved))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceStatResolved, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceAnimationStarted))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceAnimationStarted, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceAnimationCompleted))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceAnimationCompleted, this);
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceContinueRequested))
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceContinueRequested, this);
}

void UIFSMComponent::Start()
{
	FSMComponent::Start();

	m_HasTurnEndRequestAction = GraphHasAction(GetGraph(), "UI_RequestTurnEnd");
	m_PendingDiceStatRollRequest = false;
	m_PendingDiceStatResolved = false;
	m_ActiveDiceAnimationCount = 0;

	GetEventDispatcher().AddListener(EventType::Pressed, this);
	GetEventDispatcher().AddListener(EventType::KeyDown, this);
	GetEventDispatcher().AddListener(EventType::UICloseRequested, this);
	GetEventDispatcher().AddListener(EventType::UIGoToTitleRequested, this);
	GetEventDispatcher().AddListener(EventType::UIHovered, this);
	GetEventDispatcher().AddListener(EventType::Released, this);
	GetEventDispatcher().AddListener(EventType::UIDragged, this);
	GetEventDispatcher().AddListener(EventType::UIDoubleClicked, this);
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
	GetEventDispatcher().AddListener(EventType::PlayerDoorInteract, this);
	GetEventDispatcher().AddListener(EventType::PlayerDoorCancel, this);
	GetEventDispatcher().AddListener(EventType::PlayerDoorSuccess, this);
	GetEventDispatcher().AddListener(EventType::PlayerDoorFail, this);
	GetEventDispatcher().AddListener(EventType::PlayerShopOpen, this);
	GetEventDispatcher().AddListener(EventType::PlayerShopClose, this);
	GetEventDispatcher().AddListener(EventType::VendingOfferUpdated, this);
	GetEventDispatcher().AddListener(EventType::ShopMoneyOk, this);
	GetEventDispatcher().AddListener(EventType::ShopMoneyFail, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceRoll, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceUIOpen, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceUIReset, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceRollRequested, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceRollApplied, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceTotalsApplied, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceResultShown, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceUIClose, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceDecisionRequested, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceDecisionResult, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceInitiativeResolved, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceStatRollRequested, this);;
	GetEventDispatcher().AddListener(EventType::PlayerDiceTypeDetermined, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceStatResolved, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceAnimationStarted, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceAnimationCompleted, this);
	GetEventDispatcher().AddListener(EventType::PlayerDiceContinueRequested, this);

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

void UIFSMComponent::Update(float deltaTime)
{
	FSMComponent::Update(deltaTime);

	// 안전장치: 어떤 이유로 마지막 PlayerDiceAnimationCompleted가 누락돼도
	// pending 상태가 남아 버튼/전이가 막히지 않도록 업데이트 단계에서 복구한다.
	if (!m_PendingDiceStatResolved)
	{
		return;
	}

	if (m_ActiveDiceAnimationCount > 0)
	{
		return;
	}

	if (GetCurrentStateName() != "StatRolling")
	{
		m_PendingDiceStatResolved = false;
		return;
	}

	m_PendingDiceStatResolved = false;
	HandleEventByName("Player_DiceStatResolved", nullptr);
	if (GetCurrentStateName() == "StatResolved")
	{
		HandleEventByName("Player_DiceAnimationCompleted", nullptr);
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
		

	if (type == EventType::PlayerDiceStatRollRequested)
	{
		m_PendingDiceStatRollRequest = true;
	}
	else if (type == EventType::PlayerDiceDecisionResult)
	{
		// 일부 UI FSM 데이터는 DecisionReady 상태에서
		// DecisionResult를 먼저 받거나, DecisionReady->Rolling 전이를
		// Player_DiceDecisionRequested에만 걸어둔다.
		// (예: DicisionRolling 전이만 존재하는 구형 데이터)
		// 이 경우 흐름이 멈추지 않도록 보조 이벤트를 재발행한다.
		if (GetCurrentStateName() == "DecisionReady")
		{
			HandleEventByName("Player_DiceDecisionRequested", nullptr);
			HandleEventByName("Player_DiceRollRequested", nullptr);
		}
	}
	else if (type == EventType::PlayerDiceUIReset || type == EventType::PlayerDiceUIClose)
	{
		m_PendingDiceStatRollRequest = false;
		m_PendingDiceStatResolved = false;
		m_ActiveDiceAnimationCount = 0;
	}

	if (type == EventType::PlayerDiceStatResolved && m_ActiveDiceAnimationCount > 0)
	{
		// StatResolved가 롤링 애니메이션 종료 전에 들어오면
		// 마지막 애니메이션만 보이는 것처럼 보일 수 있어
		// 남은 애니메이션 종료 후 전이를 지연 처리한다.
		m_PendingDiceStatResolved = true;
		return;
	}

	if (type == EventType::PlayerDiceAnimationStarted)
	{
		++m_ActiveDiceAnimationCount;
	}
	else if (type == EventType::PlayerDiceAnimationCompleted)
	{
		if (m_ActiveDiceAnimationCount > 0)
		{
			--m_ActiveDiceAnimationCount;
			if (m_ActiveDiceAnimationCount > 0)
			{
				return;
			}
		}

		if (m_PendingDiceStatResolved)
		{
			m_PendingDiceStatResolved = false;
			HandleEventByName("Player_DiceStatResolved", nullptr);

			// 마지막 완료 이벤트를 StatResolved 전이에 소비하면
			// StatResolved -> StatDone( Player_DiceAnimationCompleted )이
			// 더 이상 들어오지 않아 버튼이 비활성으로 멈출 수 있다.
			// 방금 완료 이벤트를 동일 프레임에 다시 전달해 후속 전이를 보장한다.
			if (GetCurrentStateName() == "StatResolved")
			{
				HandleEventByName("Player_DiceAnimationCompleted", nullptr);
			}
			return;
		}
	}

	if (m_PendingDiceStatResolved
		&& m_ActiveDiceAnimationCount <= 0
		&& GetCurrentStateName() == "StatRolling")
	{
		m_PendingDiceStatResolved = false;
		HandleEventByName("Player_DiceStatResolved", nullptr);
		if (GetCurrentStateName() == "StatResolved")
		{
			HandleEventByName("Player_DiceAnimationCompleted", nullptr);
		}
	}

	if (type == EventType::PlayerDiceUIOpen
		|| type == EventType::PlayerDiceUIReset
		|| type == EventType::PlayerDiceRollRequested
		|| type == EventType::PlayerDiceDecisionRequested
		|| type == EventType::PlayerDiceDecisionFaceRolled
		|| type == EventType::PlayerDiceDecisionResult
		|| type == EventType::PlayerDiceStatRollRequested
		|| type == EventType::PlayerDiceTypeDetermined
		|| type == EventType::PlayerDiceStatResolved
		|| type == EventType::PlayerDiceContinueRequested
		|| type == EventType::PlayerDiceUIClose)
	{
	}

	HandleEventByName(*eventName, data);

	// Dice UI recovery: if stat-roll request arrived too early (before root reached
	// DecisionDone), re-dispatch once root is ready so flow can proceed to StatRolling.
	if (m_PendingDiceStatRollRequest && GetCurrentStateName() == "DecisionDone")
	{
		HandleEventByName("Player_DiceStatRollRequested", nullptr);
	}

	const std::string& currentStateName = GetCurrentStateName();
	if (currentStateName == "StatRolling"
		|| currentStateName == "StatResolved"
		|| currentStateName == "StatDone"
		|| currentStateName == "ClosePending"
		|| currentStateName == "Hidden")
	{
		m_PendingDiceStatRollRequest = false;
		if (currentStateName != "StatRolling")
		{
			m_PendingDiceStatResolved = false;
		}
	}
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
	case EventType::PlayerDoorCancel:
		return std::string("Player_DoorCancel");
	case EventType::PlayerDoorSuccess:
		return std::string("Player_DoorSuccess");
	case EventType::PlayerDoorFail:
		return std::string("Player_DoorFail");
	case EventType::PlayerShopOpen:
		return std::string("Player_ShopOpen");
	case EventType::PlayerShopClose:
		return std::string("Player_ShopClose");
	case EventType::VendingOfferUpdated:
		return std::string("UI_VendingOfferUpdated");
	case EventType::ShopMoneyOk:
		return std::string("Shop_MoneyOk");
	case EventType::ShopMoneyFail:
		return std::string("Shop_MoneyFail");
	case EventType::PlayerDiceRoll:
		return std::string("Player_DiceRoll");
	case EventType::PlayerDiceUIOpen:
		return std::string("Player_DiceUIOpen");
	case EventType::PlayerDiceUIReset:
		return std::string("Player_DiceUIReset");
	case EventType::PlayerDiceRollRequested:
		return std::string("Player_DiceRollRequested");
	case EventType::PlayerDiceRollApplied:
		return std::string("Player_DiceRollApplied");
	case EventType::PlayerDiceTotalsApplied:
		return std::string("Player_DiceTotalsApplied");
	case EventType::PlayerDiceResultShown:
		return std::string("Player_DiceResultShown");
	case EventType::PlayerDiceUIClose:
		return std::string("Player_DiceUIClose");
	case EventType::PlayerDiceDecisionRequested:
		return std::string("Player_DiceDecisionRequested");
	case EventType::PlayerDiceDecisionResult:
		return std::string("Player_DiceDecisionResult");
	case EventType::PlayerDiceInitiativeResolved:
		return std::string("Player_DiceInitiativeResolved");
	case EventType::PlayerDiceStatRollRequested:
		return std::string("Player_DiceStatRollRequested");
	case EventType::PlayerDiceTypeDetermined:
		return std::string("Player_DiceTypeDetermined");
	case EventType::PlayerDiceStatResolved:
		return std::string("Player_DiceStatResolved");
	case EventType::PlayerDiceAnimationStarted:
		return std::string("Player_DiceAnimationStarted");
	case EventType::PlayerDiceAnimationCompleted:
		return std::string("Player_DiceAnimationCompleted");
	case EventType::PlayerDiceContinueRequested:
		return std::string("Player_DiceContinueRequested");
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
	case EventType::UICloseRequested:
		return std::string("UI_CloseRequested");
	case EventType::UIGoToTitleRequested:
		return std::string("UI_GoToTitleRequested");
	case EventType::KeyDown:
	{
		const auto* keyData = static_cast<const Events::KeyEvent*>(data);
		if (!keyData || keyData->key != VK_ESCAPE)
		{
			return std::nullopt;
		}
		return std::string("UI_EscapePressed");
	}
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
