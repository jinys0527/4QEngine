#include "UIDicePanelComponent.h"
#include "Event.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"
#include "UIObject.h"
#include "UIDiceDisplayComponent.h"
#include "UIDiceRollAnimationComponent.h"

REGISTER_UI_COMPONENT(UIDicePanelComponent)
REGISTER_PROPERTY(UIDicePanelComponent, Enabled)
REGISTER_PROPERTY(UIDicePanelComponent, Slots)
REGISTER_PROPERTY(UIDicePanelComponent, ActiveDiceType)
REGISTER_PROPERTY(UIDicePanelComponent, AutoVisibility)
REGISTER_PROPERTY(UIDicePanelComponent, ApplyDecisionD20OnRequest)

void UIDicePanelComponent::Start()
{
	if (auto* scene = GetScene())
	{
		auto& services = scene->GetServices();
		if (services.Has<UIManager>())
		{
			m_UIManager = &services.Get<UIManager>();
		}
	}

	m_Dispatcher = &GetEventDispatcher();
	m_Dispatcher->AddListener(EventType::PlayerDiceDecisionRequested, this);
	m_Dispatcher->AddListener(EventType::PlayerDiceStatRollRequested, this);
	m_Dispatcher->AddListener(EventType::PlayerDiceTypeDetermined, this);
	m_Dispatcher->AddListener(EventType::PlayerDiceDecisionFaceRolled, this);
	m_BindingsDirty = true;
}

UIDicePanelComponent::~UIDicePanelComponent()
{
	if (m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::PlayerDiceDecisionRequested))
	{
		m_Dispatcher->RemoveListener(EventType::PlayerDiceDecisionRequested, this);
	}
	if (m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::PlayerDiceStatRollRequested))
	{
		m_Dispatcher->RemoveListener(EventType::PlayerDiceStatRollRequested, this);
	}
	if (m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::PlayerDiceTypeDetermined))
	{
		m_Dispatcher->RemoveListener(EventType::PlayerDiceTypeDetermined, this);
	}
	if (m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::PlayerDiceDecisionFaceRolled))
	{
		m_Dispatcher->RemoveListener(EventType::PlayerDiceDecisionFaceRolled, this);
	}
}

void UIDicePanelComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
	(void)deltaTime;

	if (!m_Enabled)
	{
		return;
	}

	if (!m_BindingsDirty)
	{
		return;
	}

	if (!CanApplySlotsImmediately())
	{
		return;
	}

	for (const auto& slot : m_Slots)
	{
		if (slot.objectName.empty())
		{
			continue;
		}

		if (auto* target = FindUIObject(slot.objectName))
		{
			ApplySlot(*target, slot);
		}
	}

	m_BindingsDirty = false;
}

void UIDicePanelComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);

	if (!m_Enabled)
	{
		return;
	}

	const auto hasDecisionContext = [&]()
		{
			for (const auto& slot : m_Slots)
			{
				if (slot.diceContext.find("InitiativeDecisionRoll_") != std::string::npos)
				{
					return true;
				}
			}
			return false;
		};


	if (type == EventType::PlayerDiceDecisionRequested)
	{
		if (m_ApplyDecisionD20OnRequest && hasDecisionContext() && m_ActiveDiceType != "D20")
		{
			SetActiveDiceType("D20");
		}
		return;
	}

	if (type == EventType::PlayerDiceStatRollRequested)
	{
		if (!m_PendingDiceType.empty())
		{
			SetActiveDiceType(m_PendingDiceType);
		}
		return;
	}

	if (type == EventType::PlayerDiceDecisionFaceRolled)
	{
		const auto* payload = static_cast<const Events::DiceDecisionFaceEvent*>(data);
		if (!payload)
		{
			return;
		}

		const auto hasMatchingContext = [&]()
			{
				for (const auto& slot : m_Slots)
				{
					if (slot.diceContext == payload->context)
					{
						return true;
					}
				}
				return false;
			};

		if (!hasMatchingContext())
		{
			return;
		}

		const int d20 = payload->value;
		int diceSides = 0;
		if (d20 <= 5)
		{
			diceSides = 12;
		}
		else if (d20 <= 10)
		{
			diceSides = 8;
		}
		else if (d20 <= 15)
		{
			diceSides = 6;
		}
		else
		{
			diceSides = 4;
		}

		m_PendingDiceType = "D" + std::to_string(diceSides);

		if (!m_ApplyDecisionD20OnRequest && hasDecisionContext() && m_ActiveDiceType != "D20")
		{
			SetActiveDiceType("D20");
		}
		return;
	}

	if (type != EventType::PlayerDiceTypeDetermined)
	{
		return;
	}

	if (hasDecisionContext())
	{
		return;
	}

	const auto* payload = static_cast<const Events::DiceRollEvent*>(data);
	if (!payload)
	{
		return;
	}

	const int diceSides = payload->diceSides > 0 ? payload->diceSides : payload->value;
	if (diceSides <= 0)
	{
		return;
	}

	m_PendingDiceType = "D" + std::to_string(diceSides);
}

void UIDicePanelComponent::SetEnabled(const bool& enabled)
{
	if (m_Enabled == enabled)
	{
		return;
	}

	m_Enabled = enabled;
	m_BindingsDirty = true;
}

void UIDicePanelComponent::SetSlots(std::vector<UIDicePanelSlot> slots)
{
	m_Slots = std::move(slots);
	m_BindingsDirty = true;
	if (CanApplySlotsImmediately())
	{
		ApplySlotsImmediate();
	}
}

void UIDicePanelComponent::SetActiveDiceType(const std::string& type)
{
	if (m_ActiveDiceType == type)
	{
		return;
	}

	m_ActiveDiceType = type;
	m_BindingsDirty = true;
	if (CanApplySlotsImmediately())
	{
		ResetActiveSlotValues();
		ApplySlotsImmediate();
	}
}

void UIDicePanelComponent::SetAutoVisibility(const bool& enabled)
{
	if (m_AutoVisibility == enabled)
	{
		return;
	}

	m_AutoVisibility = enabled;
	m_BindingsDirty = true;
	if (CanApplySlotsImmediately())
	{
		ApplySlotsImmediate();
	}
}

void UIDicePanelComponent::SetApplyDecisionD20OnRequest(const bool& enabled)
{
	if (m_ApplyDecisionD20OnRequest == enabled)
	{
		return;
	}

	m_ApplyDecisionD20OnRequest = enabled;
}

void UIDicePanelComponent::RefreshBindings()
{
	m_BindingsDirty = true;
	if (CanApplySlotsImmediately())
	{
		ApplySlotsImmediate();
	}
}

UIObject* UIDicePanelComponent::FindUIObject(const std::string& name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	if (!m_UIScene)
	{
		return nullptr;
	}


	auto* uiManager = GetUIManager();
	if (!uiManager)
	{
		return nullptr;
	}

	const auto* scene = GetScene();
	const std::string sceneName = scene ? scene->GetName() : std::string{};
	const std::string currentScene = uiManager->GetCurrentScene();

	if (!sceneName.empty())
	{
		auto uiObject = uiManager->FindUIObject(sceneName, name);
		if (uiObject)
		{
			return uiObject.get();
		}
	}

	if (!currentScene.empty() && currentScene != sceneName)
	{
		auto uiObject = uiManager->FindUIObject(currentScene, name);
		if (uiObject)
		{
			return uiObject.get();
		}
	}

	return nullptr;
}


void UIDicePanelComponent::ApplySlot(UIObject& object, const UIDicePanelSlot& slot) const
{
	if (m_AutoVisibility)
	{
		const bool shouldShow = m_ActiveDiceType.empty() || slot.diceType == m_ActiveDiceType;
		object.SetIsVisibleFromComponent(shouldShow);
	}

	if (auto* diceDisplay = object.GetComponent<UIDiceDisplayComponent>())
	{
		if (m_AutoVisibility)
		{
			if (m_ActiveDiceType.empty())
			{
				diceDisplay->SetEnabled(true);
			}
			else
			{
				diceDisplay->SetEnabled(slot.diceType == m_ActiveDiceType);
			}
		}

		if (!slot.diceType.empty())
		{
			diceDisplay->SetDiceType(slot.diceType);
		}

		if (!slot.diceContext.empty())
		{
			diceDisplay->SetDiceContext(slot.diceContext);
		}
	}

	if (auto* diceAnim = object.GetComponent<UIDiceRollAnimationComponent>())
	{
		if (m_AutoVisibility)
		{
			if (m_ActiveDiceType.empty())
			{
				diceAnim->SetEnabled(slot.applyAnimation);
			}
			else
			{
				diceAnim->SetEnabled(slot.applyAnimation && slot.diceType == m_ActiveDiceType);
			}
		}
		else
		{
			diceAnim->SetEnabled(slot.applyAnimation);
		}

		if (!slot.diceContext.empty())
		{
			diceAnim->SetDiceContext(slot.diceContext);
		}
	}
}

bool UIDicePanelComponent::CanApplySlotsImmediately() const
{
	if (!m_Enabled)
	{
		return false;
	}

	if (!m_Dispatcher)
	{
		// Scene load/deserialize 단계에서는 Start 이전이라 즉시 적용을 건너뛴다.
		return false;
	}

	if (!GetOwner())
	{
		return false;
	}

	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!scene || !uiManager)
	{
		return false;
	}

	const auto& currentScene = uiManager->GetCurrentScene();
	if (!currentScene.empty() && currentScene != scene->GetName())
	{
		return false;
	}

	return true;
}

void UIDicePanelComponent::ApplySlotsImmediate() const
{
	if (!CanApplySlotsImmediately())
	{
		return;
	}

	if (!GetOwner())
	{
		return;
	}

	if (!GetScene() || !GetUIManager())
	{
		return;
	}

	for (const auto& slot : m_Slots)
	{
		if (slot.objectName.empty())
		{
			continue;
		}

		if (auto* target = FindUIObject(slot.objectName))
		{
			ApplySlot(*target, slot);
		}
	}
}

void UIDicePanelComponent::ResetActiveSlotValues() const
{
	if (m_ActiveDiceType.empty())
	{
		return;
	}

	for (const auto& slot : m_Slots)
	{
		if (slot.objectName.empty() || slot.diceType != m_ActiveDiceType)
		{
			continue;
		}

		if (auto* target = FindUIObject(slot.objectName))
		{
			if (auto* diceDisplay = target->GetComponent<UIDiceDisplayComponent>())
			{
				diceDisplay->SetValue(0);
			}
		}
	}
}
