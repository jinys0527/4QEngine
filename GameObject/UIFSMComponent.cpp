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
#include "CombatManager.h"
#include "PlayerDoorFSMComponent.h"
#include "PlayerFSMComponent.h"
#include "PlayerShopFSMComponent.h"
#include "AssetLoader.h"
#include "GameDataRepository.h"
#include "UIManager.h"
#include "UIImageComponent.h"
#include "ServiceRegistry.h"
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

	int ResolveVendingHoverIndex(const std::string& objectName)
	{
		auto resolveByPrefix = [&](const std::string& prefix)
			{
				if (objectName.rfind(prefix, 0) != 0)
				{
					return 0;
				}

				const size_t indexStart = prefix.size();
				size_t indexEnd = indexStart;
				while (indexEnd < objectName.size() && std::isdigit(static_cast<unsigned char>(objectName[indexEnd])))
				{
					++indexEnd;
				}

				if (indexEnd == indexStart || indexEnd != objectName.size())
				{
					return 0;
				}

				const int parsed = std::stoi(objectName.substr(indexStart, indexEnd - indexStart));
				return (parsed >= 1 && parsed <= 6) ? parsed : 0;
			};

		for (const std::string prefix : { "VendingSlot", "VendingItem", "ItemImage", "ItemIcon", "Vending" })
		{
			const int resolved = resolveByPrefix(prefix);
			if (resolved > 0)
			{
				return resolved;
			}
		}


		return 0;
	}

	bool IsDiceActionButtonName(const std::string& ownerName)
	{
		return ownerName == "DiceRollBtn"
			|| ownerName == "DiceStatBtn"
			|| ownerName == "DiceContinueBtn";
	}

	using DiceClock = std::chrono::steady_clock;
	constexpr auto kDiceClickDebounce = std::chrono::milliseconds(650);

	std::unordered_map<std::string, DiceClock::time_point>& GetDiceLastClickByOwner()
	{
		static std::unordered_map<std::string, DiceClock::time_point> s_LastClickByOwner;
		return s_LastClickByOwner;
	}

	std::unordered_map<std::string, DiceClock::time_point>& GetDicePendingReenableByOwner()
	{
		static std::unordered_map<std::string, DiceClock::time_point> s_PendingReenableByOwner;
		return s_PendingReenableByOwner;
	}

	void MarkDicePendingReenable(const std::string& ownerName)
	{
		if (!IsDiceActionButtonName(ownerName))
		{
			return;
		}

		auto& lastClickByOwner = GetDiceLastClickByOwner();
		auto it = lastClickByOwner.find(ownerName);
		if (it == lastClickByOwner.end())
		{
			return;
		}

		GetDicePendingReenableByOwner()[ownerName] = it->second + kDiceClickDebounce;
	}

	bool IsDiceReenableReady(const std::string& ownerName)
	{
		auto& pendingReenableByOwner = GetDicePendingReenableByOwner();
		auto it = pendingReenableByOwner.find(ownerName);
		if (it == pendingReenableByOwner.end())
		{
			return false;
		}

		if (DiceClock::now() < it->second)
		{
			return false;
		}

		pendingReenableByOwner.erase(it);
		return true;
	}

	bool HasDicePendingReenable(const std::string& ownerName)
	{
		return GetDicePendingReenableByOwner().find(ownerName) != GetDicePendingReenableByOwner().end();
	}


	bool ShouldThrottleDiceUIButtonClick(const std::string& ownerName)
	{
		if (!IsDiceActionButtonName(ownerName))
		{
			return false;
		}

		auto& lastClickByOwner = GetDiceLastClickByOwner();
		const auto now = DiceClock::now();
		auto it = lastClickByOwner.find(ownerName);
		if (it != lastClickByOwner.end() && (now - it->second) < kDiceClickDebounce)
		{
			return true;
		}

		lastClickByOwner[ownerName] = now;
		return false;
	}

	bool IsCombatDiceFlowActive(UIFSMComponent* component)
	{
		if (!component)
		{
			return false;
		}

		auto* owner = component->GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;
		auto* combatManager = gameManager ? gameManager->GetCombatManager() : nullptr;
		return combatManager && combatManager->IsDiceFlowActive();
	}

	bool IsAnyVendingHoverCandidateHit(Scene* scene, int slotIndex, const POINT& mousePos)
	{
		if (!scene || slotIndex <= 0 || slotIndex > 6)
		{
			return false;
		}

		auto& services = scene->GetServices();
		if (!services.Has<UIManager>())
		{
			return false;
		}

		auto& uiManager = services.Get<UIManager>();
		const std::string sceneName = scene->GetName();
		const std::string suffix = std::to_string(slotIndex);
		const std::array<std::string, 5> candidates =
		{
			"ItemImage" + suffix,
			"VendingSlot" + suffix,
			"VendingItem" + suffix,
			"ItemIcon" + suffix,
			"Vending" + suffix
		};

		for (const auto& candidate : candidates)
		{
			auto uiObject = uiManager.FindUIObject(sceneName, candidate);
			if (!uiObject || !uiObject->IsVisible() || !uiObject->HasBounds())
			{
				continue;
			}

			if (uiObject->HitCheck(mousePos))
			{
				return true;
			}
		}

		return false;
	}

	void TriggerVendingInfoHoverEvent(Scene* scene, int slotIndex, bool isHovered)
	{
		if (!scene || slotIndex <= 0 || slotIndex > 6)
		{
			return;
		}

		auto& services = scene->GetServices();
		if (!services.Has<UIManager>())
		{
			return;
		}

		auto& uiManager = services.Get<UIManager>();
		const std::string sceneName = scene->GetName();
		const std::string suffix = std::to_string(slotIndex);
		const std::string eventName = std::string(isHovered ? "UI_RequestItemInfoShow_Vending" : "UI_RequestItemInfoHide_Vending") + suffix;

		const std::array<std::string, 2> infoCandidates =
		{
			"ItemInfo" + suffix,
			"VendingSlot" + suffix + "Info"
		};

		for (const auto& infoName : infoCandidates)
		{
			auto infoObject = uiManager.FindUIObject(sceneName, infoName);
			if (!infoObject)
			{
				continue;
			}

			if (auto* infoFsm = infoObject->GetComponent<UIFSMComponent>())
			{
				infoFsm->TriggerEventByName(eventName);
				break;
			}
		}
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

	std::string NormalizePath(std::string value)
	{
		std::replace(value.begin(), value.end(), '\\', '/');
		return value;
	}

	bool EndsWithPath(const std::string& value, const std::string& suffix)
	{
		if (suffix.empty() || value.size() < suffix.size())
		{
			return false;
		}
		return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
	}

	TextureHandle ResolveTextureByPath(AssetLoader& assetLoader, const std::string& iconPath)
	{
		if (iconPath.empty())
		{
			return TextureHandle::Invalid();
		}

		const std::string normalizedPath = NormalizePath(iconPath);
		const auto& keyToHandle = assetLoader.GetTextures().GetKeyToHandle();
		auto direct = keyToHandle.find(normalizedPath);
		if (direct != keyToHandle.end())
		{
			return direct->second;
		}

		std::string trimmedPath = normalizedPath;
		while (trimmedPath.rfind("../", 0) == 0)
		{
			trimmedPath.erase(0, 3);
			auto trimmed = keyToHandle.find(trimmedPath);
			if (trimmed != keyToHandle.end())
			{
				return trimmed->second;
			}
		}

		const size_t filenameStart = normalizedPath.find_last_of('/');
		const std::string filename = (filenameStart == std::string::npos)
			? normalizedPath
			: normalizedPath.substr(filenameStart + 1);

		for (const auto& [key, handle] : keyToHandle)
		{
			const std::string normalizedKey = NormalizePath(key);
			if (normalizedKey == normalizedPath || EndsWithPath(normalizedKey, normalizedPath) || EndsWithPath(normalizedKey, filename))
			{
				return handle;
			}
		}

		return TextureHandle::Invalid();
	}

	UIImageComponent* FindImageOnObjectOrChildren(UIManager& uiManager, const std::string& sceneName, const std::string& objectName, std::shared_ptr<UIObject>& outTarget)
	{
		outTarget = uiManager.FindUIObject(sceneName, objectName);
		if (!outTarget)
		{
			return nullptr;
		}

		if (auto* image = outTarget->GetComponent<UIImageComponent>())
		{
			return image;
		}

		auto& sceneObjects = uiManager.GetUIObjects()[sceneName];
		for (auto& [name, obj] : sceneObjects)
		{
			if (!obj || obj->GetParentName() != objectName)
			{
				continue;
			}
			if (auto* image = obj->GetComponent<UIImageComponent>())
			{
				outTarget = obj;
				return image;
			}
		}

		return nullptr;
	}

	std::vector<std::string> GetVendingSlotNameCandidates(int index)
	{
		const int slot = index + 1;
		return {
			"ItemImage" + std::to_string(slot)
		};
	}


	UIImageComponent* FindVendingSlotImageByParent(UIManager& uiManager,
		const std::string& sceneName,
		const std::string& vendingObjectName,
		int index,
		std::shared_ptr<UIObject>& outTarget)
	{
		if (vendingObjectName.empty())
		{
			return nullptr;
		}

		auto sceneIt = uiManager.GetUIObjects().find(sceneName);
		if (sceneIt == uiManager.GetUIObjects().end())
		{
			return nullptr;
		}

		std::vector<std::shared_ptr<UIObject>> slotCandidates;
		for (auto& [name, object] : sceneIt->second)
		{
			if (!object || object->GetParentName() != vendingObjectName)
			{
				continue;
			}

			const std::string lower = ToLower(name);
			if (lower.find("close") != std::string::npos || lower.find("xbutton") != std::string::npos)
			{
				continue;
			}

			if (object->GetComponent<UIImageComponent>() || object->GetComponent<UIButtonComponent>())
			{
				slotCandidates.push_back(object);
			}
		}

		std::sort(slotCandidates.begin(), slotCandidates.end(),
			[](const std::shared_ptr<UIObject>& a, const std::shared_ptr<UIObject>& b)
			{
				if (!a || !b)
				{
					return static_cast<bool>(a);
				}
				return a->GetName() < b->GetName();
			});

		if (index < 0 || index >= static_cast<int>(slotCandidates.size()))
		{
			return nullptr;
		}

		outTarget = slotCandidates[index];
		if (!outTarget)
		{
			return nullptr;
		}

		if (auto* image = outTarget->GetComponent<UIImageComponent>())
		{
			return image;
		}

		for (auto& [name, object] : sceneIt->second)
		{
			if (!object || object->GetParentName() != outTarget->GetName())
			{
				continue;
			}
			if (auto* image = object->GetComponent<UIImageComponent>())
			{
				outTarget = object;
				return image;
			}
		}

		return nullptr;
	}

	void UpdateVendingSlotIcon(Scene* scene, UIManager& uiManager, AssetLoader& assetLoader, GameDataRepository& repo, const Events::VendingOfferUpdatedEvent& payload, int index)
	{
		if (!scene)
		{
			return;
		}

		TextureHandle icon = TextureHandle::Invalid();
		if (index < static_cast<int>(payload.itemIds.size()))
		{
			const int itemId = payload.itemIds[index];
			if (const auto* item = repo.GetItem(itemId))
			{
				icon = ResolveTextureByPath(assetLoader, item->iconPath);
			}
		}

		const auto candidates = GetVendingSlotNameCandidates(index);
		const std::string sceneName = scene->GetName();
		for (const auto& slotName : candidates)
		{
			std::shared_ptr<UIObject> target;
			auto* image = FindImageOnObjectOrChildren(uiManager, sceneName, slotName, target);
			if (!image || !target)
			{
				continue;
			}

			if (icon.IsValid())
			{
				image->SetTextureHandle(icon);
				target->SetIsVisible(true);
			}
			else
			{
				target->SetIsVisible(false);
			}
			return;
		}

		std::shared_ptr<UIObject> fallbackTarget;
		auto* fallbackImage = FindVendingSlotImageByParent(uiManager, sceneName, payload.vendingObjectName, index, fallbackTarget);
		if (!fallbackImage || !fallbackTarget)
		{
			return;
		}

		if (icon.IsValid())
		{
			fallbackImage->SetTextureHandle(icon);
			fallbackTarget->SetIsVisible(true);
		}
		else
		{
			fallbackTarget->SetIsVisible(false);
		}
	}

	void UpdateVendingOfferUI(Scene* scene, const Events::VendingOfferUpdatedEvent* payload)
	{
		if (!scene || !payload)
		{
			return;
		}

		auto& services = scene->GetServices();
		if (!services.Has<UIManager>() || !services.Has<AssetLoader>() || !services.Has<GameDataRepository>())
		{
			return;
		}

		auto& uiManager = services.Get<UIManager>();
		auto& assetLoader = services.Get<AssetLoader>();
		auto& repo = services.Get<GameDataRepository>();

		for (int i = 0; i < 6; ++i)
		{
			UpdateVendingSlotIcon(scene, uiManager, assetLoader, repo, *payload, i);
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
		"UI_RequestExplorationTurnStart",
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
		"UI_RequestInventoryTrash",
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
	eventRegistry.RegisterEvent({ "UI_ExplorePlayerTurnRequested", "UI" });
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
	eventRegistry.RegisterEvent({ "Player_InventoryTrash", "UI" });
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

			const Phase phase = gameManager->GetPhase();
			const bool canEndExplorationTurn = phase == Phase::ExplorationLoop
				&& gameManager->GetExplorationTurnState() == ExplorationTurnState::PlayerTurn;
			const bool canEndCombatTurn = phase == Phase::TurnBasedCombat
				&& gameManager->GetCombatTurnState() == CombatTurnState::PlayerTurn;

			if (!canEndExplorationTurn && !canEndCombatTurn)
			{
				UpdateTurnEndButtonState(Turn::EnemyTurn);
				return;
			}

			if (canEndExplorationTurn)
			{
				GetEventDispatcher().Dispatch(EventType::ExploreTurnEnded, nullptr);
			}
			else
			{
				GetEventDispatcher().Dispatch(EventType::PlayerTurnEndRequested, nullptr);
			}
		});

	BindActionHandler("UI_RequestExplorationTurnStart", [this](const FSMAction&)
		{
			GetEventDispatcher().Dispatch(EventType::ExplorePlayerTurnRequested, nullptr);
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
			std::cout << "[UIFSM][Trace] Action UI_RequestDiceContinue -> dispatch PlayerDiceContinueRequested"
				<< " currentState=" << GetCurrentStateName() << std::endl;
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
			else
			{
				// 레거시 UI 데이터에서 `event: "None"` 대신 빈 문자열을 전달하는 경우가 있어
				// Process -> Idle 전이를 위해 명시적으로 None 이벤트를 보낸다.
				DispatchEvent("None");
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
			std::cout << "UI_RequestShopClose\n";
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			// Shop_Close는 Player 메인 FSM이 아니라 Shop 서브 FSM에서 처리된다.
			DispatchPlayerSubEvent(scene, "Shop", "Shop_Close");
			// 일부 UI 프리팹에서 Shop 서브 FSM 연결이 누락되어도 닫기 반응은 보장한다.
			GetEventDispatcher().Dispatch(EventType::PlayerShopClose, nullptr);
			DispatchEvent("None");
		});

	BindActionHandler("UI_RequestDoorCancel", [this](const FSMAction& action)
		{
			std::cout << "[UIFSM][Trace] Action UI_RequestDoorCancel -> dispatch PlayerDoorCancel + Door_Complete" << std::endl;
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;

			// 닫기 버튼은 즉시 반응해야 하므로 UI 종료 이벤트를 바로 발행한다.
			GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
			// Player FSM을 Door 상태에서 확실히 복귀시켜 다음 Door_Interact 재진입을 보장한다.
			DispatchPlayerEvent(scene, "Door_Complete");
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

	BindActionHandler("UI_RequestInventoryTrash", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* scene = owner ? owner->GetScene() : nullptr;
			DispatchPlayerEvent(scene, "Player_InventoryTrash");
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
	GetEventDispatcher().AddListener(EventType::ExplorePlayerTurnRequested, this);
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

	const std::string ownerName = GetOwner() ? GetOwner()->GetName() : std::string{};
	if (GetCurrentStateName() == "Disabled"
		&& HasDicePendingReenable(ownerName)
		&& IsDiceReenableReady(ownerName))
	{
		HandleEventByName("Player_DiceAnimationCompleted", nullptr);
	}

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

	if (type == EventType::UIHovered)
	{
		auto* owner = GetOwner();
		auto* uiObject = owner ? dynamic_cast<UIObject*>(owner) : nullptr;
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		const bool isHovered = (uiObject && mouseData && uiObject->IsVisible() && uiObject->HasBounds())
			? uiObject->HitCheck(mouseData->pos)
			: false;

		if (m_IsHovering != isHovered)
		{
			{
				const int vendingIndex = owner ? ResolveVendingHoverIndex(owner->GetName()) : 0;
				if (vendingIndex > 0)
				{
					auto* scene = owner ? owner->GetScene() : nullptr;
					if (isHovered)
					{
						TriggerVendingInfoHoverEvent(scene, vendingIndex, true);
					}
					else if (!mouseData || !IsAnyVendingHoverCandidateHit(scene, vendingIndex, mouseData->pos))
					{
						TriggerVendingInfoHoverEvent(scene, vendingIndex, false);
					}
				}
			}
			m_IsHovering = isHovered;
		}
	}


	if (type == EventType::VendingOfferUpdated)
	{
		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		UpdateVendingOfferUI(scene, static_cast<const Events::VendingOfferUpdatedEvent*>(data));
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

		const std::string ownerName = GetOwner() ? GetOwner()->GetName() : std::string{};
		if (IsDiceActionButtonName(ownerName)
			&& GetCurrentStateName() == "Disabled"
			&& IsCombatDiceFlowActive(this))
		{
			// 전투 주사위 플로우 중에는 마지막 클릭 후 디바운스 시간(650ms)이
			// 지나기 전까지는 조기 복귀를 막고, 시간이 지나면 Update에서 재활성화한다.
			MarkDicePendingReenable(ownerName);
			return;
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

	if (type == EventType::UIHovered)
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
	case EventType::ExplorePlayerTurnRequested:
		return std::string("UI_ExplorePlayerTurnRequested");
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

	const std::string stateBeforeDispatch = GetCurrentStateName();

	const std::string ownerName = GetOwner() ? GetOwner()->GetName() : std::string{};
	if (eventName == "UI_Clicked" && ShouldThrottleDiceUIButtonClick(ownerName))
	{
		std::cout << "[UIFSM][Trace] throttle rapid dice click owner=" << ownerName
			<< " event=" << eventName << std::endl;
		return;
	}

	const bool isDiceDoorEvent = eventName == "UI_Released"
		|| eventName == "UI_Clicked"
		|| eventName == "Player_DoorInteract"
		|| eventName == "Player_DoorCancel"
		|| eventName == "Player_DoorSuccess"
		|| eventName == "Player_DoorFail"
		|| eventName == "Player_DiceUIOpen"
		|| eventName == "Player_DiceUIReset"
		|| eventName == "Player_DiceRollRequested"
		|| eventName == "Player_DiceDecisionRequested"
		|| eventName == "Player_DiceDecisionResult"
		|| eventName == "Player_DiceStatRollRequested"
		|| eventName == "Player_DiceStatResolved"
		|| eventName == "Player_DiceAnimationStarted"
		|| eventName == "Player_DiceAnimationCompleted"
		|| eventName == "Player_DiceContinueRequested"
		|| eventName == "Player_DiceUIClose";
	
	const bool isLikelyDoorDiceUI = ownerName.find("Door") != std::string::npos
		|| ownerName.find("Dice") != std::string::npos;

	auto dispatchEventAndCallbacks = [this, data](const std::string& dispatchEventName)
		{
			DispatchEvent(dispatchEventName);

			for (const auto& entry : m_EventCallbacks)
			{
				if (entry.eventName != dispatchEventName)
				{
					continue;
				}

				auto it = m_Callbacks.find(entry.callbackId);
				if (it != m_Callbacks.end())
				{
					it->second(dispatchEventName, data);
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
		};

	dispatchEventAndCallbacks(eventName);

	// UI 버튼 FSM 데이터가 클릭 이벤트를 UI_Released 또는 UI_Clicked 중 하나로만
	// 정의되어 있어도 동작하도록 클릭 이벤트를 상호 호환시킨다.
	// 단, UI_Released 처리에서 이미 상태 전이가 발생했다면 같은 입력으로
	// UI_Clicked까지 연속 발행되어 다음 상태 액션이 즉시 실행될 수 있으므로
	// 이 경우에는 보조 이벤트를 생략한다.
	const std::string stateAfterDispatch = GetCurrentStateName();
	const bool stateChanged = stateAfterDispatch != stateBeforeDispatch;
	if (isDiceDoorEvent && stateChanged)
	{
		std::cout << "[UIFSM][Trace] owner=" << ownerName
			<< " event=" << eventName
			<< " stateBefore=" << stateBeforeDispatch
			<< " stateAfter=" << stateAfterDispatch << std::endl;
	}

	if (eventName == "UI_Released" && stateAfterDispatch == stateBeforeDispatch)
	{
		if (isLikelyDoorDiceUI)
		{
			std::cout << "[UIFSM][Trace] Skip synthetic UI_Clicked on Door/Dice UI. state="
				<< stateAfterDispatch << " owner=" << ownerName << std::endl;
		}
		else
		{
			dispatchEventAndCallbacks("UI_Clicked");
		}
	}
	else if (eventName == "UI_Released" && isDiceDoorEvent && isLikelyDoorDiceUI)
	{
		std::cout << "[UIFSM][Trace] Skip synthetic UI_Clicked because state changed on UI_Released."
			<< " stateBefore=" << stateBeforeDispatch
			<< " stateAfter=" << stateAfterDispatch << std::endl;
	}
}
