#include "UIDicePanelComponent.h"
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

	m_BindingsDirty = true;
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
	(void)type;
	(void)data;
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
	ApplySlotsImmediate();
}

void UIDicePanelComponent::SetActiveDiceType(const std::string& type)
{
	if (m_ActiveDiceType == type)
	{
		return;
	}

	m_ActiveDiceType = type;
	m_BindingsDirty = true;
	ApplySlotsImmediate();
}

void UIDicePanelComponent::SetAutoVisibility(const bool& enabled)
{
	if (m_AutoVisibility == enabled)
	{
		return;
	}

	m_AutoVisibility = enabled;
	m_BindingsDirty = true;
	ApplySlotsImmediate();
}

void UIDicePanelComponent::RefreshBindings()
{
	m_BindingsDirty = true;
	ApplySlotsImmediate();
}

UIManager* UIDicePanelComponent::GetUIManager() const
{
	if (m_UIManager)
	{
		return m_UIManager;
	}

	if (auto* scene = GetScene())
	{
		auto& services = scene->GetServices();
		if (services.Has<UIManager>())
		{
			m_UIManager = &services.Get<UIManager>();
		}
	}

	return m_UIManager;
}

Scene* UIDicePanelComponent::GetScene() const
{
	auto* owner = GetOwner();
	return owner ? owner->GetScene() : nullptr;
}

UIObject* UIDicePanelComponent::FindUIObject(const std::string& name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!scene || !uiManager)
	{
		return nullptr;
	}

	auto uiObject = uiManager->FindUIObject(scene->GetName(), name);
	return uiObject ? uiObject.get() : nullptr;
}

void UIDicePanelComponent::ApplySlot(UIObject& object, const UIDicePanelSlot& slot) const
{
	if (m_AutoVisibility && !m_ActiveDiceType.empty())
	{
		const bool matches = slot.diceType.empty() || slot.diceType == m_ActiveDiceType;
		object.SetIsVisibleFromComponent(matches);
	}

	if (auto* diceDisplay = object.GetComponent<UIDiceDisplayComponent>())
	{
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
		diceAnim->SetEnabled(slot.applyAnimation);
		if (!slot.diceContext.empty())
		{
			diceAnim->SetDiceContext(slot.diceContext);
		}
	}
}

void UIDicePanelComponent::ApplySlotsImmediate() const
{
	if (!m_Enabled)
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
