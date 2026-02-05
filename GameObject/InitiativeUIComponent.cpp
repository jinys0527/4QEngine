﻿#include "InitiativeUIComponent.h"
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"
#include "HorizontalBox.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "UIImageComponent.h"
#include "CombatEvents.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"
#include "UIObject.h"
#include "UITextComponent.h"
#include <algorithm>
#include <vector>

REGISTER_COMPONENT(InitiativeUIComponent)
REGISTER_PROPERTY(InitiativeUIComponent, Enabled)
REGISTER_PROPERTY(InitiativeUIComponent, Scale)
REGISTER_PROPERTY_HANDLE(InitiativeUIComponent, DeadIconTexture)


InitiativeUIComponent::~InitiativeUIComponent()
{
	auto& dispatcher = GetEventDispatcher();
	dispatcher.RemoveListener(EventType::CombatEnter, this);
	dispatcher.RemoveListener(EventType::CombatInitiativeBuilt, this);
	dispatcher.RemoveListener(EventType::CombatTurnAdvanced, this);
	dispatcher.RemoveListener(EventType::CombatExit, this);
	dispatcher.RemoveListener(EventType::CombatEnded, this);
}

void InitiativeUIComponent::Start()
{
	auto& dispatcher = GetEventDispatcher();
	dispatcher.AddListener(EventType::CombatEnter, this);
	dispatcher.AddListener(EventType::CombatInitiativeBuilt, this);
	dispatcher.AddListener(EventType::CombatTurnAdvanced, this);
	dispatcher.AddListener(EventType::CombatExit, this);
	dispatcher.AddListener(EventType::CombatEnded, this);
}

void InitiativeUIComponent::Update(float deltaTime)
{
	(void)deltaTime;

	if (!m_Enabled || m_InitiativeOrder.empty())
		return;

	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!scene || !uiManager)
		return;

	const std::string sceneName = scene->GetName();
	auto box   = uiManager->FindUIObject(sceneName, "InitiativeBox");
	auto frame = uiManager->FindUIObject(sceneName, "InitiativeFrame");
	if (!box || !frame || m_OrderDirty)
	{
		RebuildInitiativeOrder();
		box   = uiManager->FindUIObject(sceneName, "InitiativeBox");
		frame = uiManager->FindUIObject(sceneName, "InitiativeFrame");
	}

	if (!box || !frame)
		return;

	auto* horizontalBox = box->GetComponent<HorizontalBox>();
	if (!horizontalBox)
		return;

	auto& slots = horizontalBox->GetSlotsMutable();
	if (slots.empty())
		return;

	for (size_t i = 0; i < slots.size(); ++i)
	{
		const UIObject* slotObject = slots[i].child;
		bool isActive = false;
		if (slotObject)
		{
			const auto it = std::find_if(m_ActorIcons.begin(), m_ActorIcons.end(),
				[slotObject](const auto& pair)
				{
					return pair.second.get() == slotObject;
				});

			if (it != m_ActorIcons.end())
			{
				isActive = it->first == m_CurrentActorId;
			} 
		}

		slots[i].layoutScale = isActive ? m_Scale : 1.0f;
		const float scale = slots[i].layoutScale > 0.0f ? slots[i].layoutScale : 1.0f;

		const float extraHeight = (scale - 1.0f) * slots[i].desiredSize.height;
		const float basePadding = 10.0f;
		slots[i].padding.left   = basePadding;
		slots[i].padding.right  = basePadding;
		slots[i].padding.top    = basePadding;
		slots[i].padding.bottom = basePadding - extraHeight;
	}

	const float spacing = horizontalBox->GetSpacing();
	float contentWidth  = 0.0f;
	float contentHeight = 0.0f;
	
	for (size_t i = 0; i < slots.size(); ++i)
	{
		const float basePadding = 10.0f;
		const float scale = slots[i].layoutScale > 0.0f ? slots[i].layoutScale : 1.0f;

		contentWidth += slots[i].desiredSize.width * scale + basePadding * 2.0f;
		contentHeight = max(contentHeight, slots[i].desiredSize.height + basePadding * 2.0f);

		if (i + 1 < slots.size())
		{
			contentWidth += spacing;
		}
	}

	const UIRect frameBounds = frame->GetBounds();
	UIRect boxBounds = box->GetBounds();
	boxBounds.width  = contentWidth;
	boxBounds.height = max(contentHeight, boxBounds.height);
	boxBounds.x = frameBounds.x + (frameBounds.width  - contentWidth) * 0.5f;
	boxBounds.y = frameBounds.y + (frameBounds.height - boxBounds.height) * 0.5f;
	box->SetBounds(boxBounds);
}

void InitiativeUIComponent::OnEvent(EventType type, const void* data)
{
	if (!m_Enabled)
		return;

	switch (type)
	{
	case EventType::CombatEnter:
		m_InCombat = true;
		SetFrameVisible(!m_InitiativeOrder.empty());
		break;
	case EventType::CombatInitiativeBuilt:
	{
		const auto* payload = static_cast<const CombatInitiativeBuiltEvent*>(data);
		if (!payload || !payload->initiativeOrder)
			return;

		m_InCombat = true;
		m_InitiativeOrder = *payload->initiativeOrder;
		m_OrderDirty = true;
		RebuildInitiativeOrder();
		break;
	}
	case EventType::CombatTurnAdvanced:
	{
		const auto* payload = static_cast<const CombatTurnAdvancedEvent*>(data);
		if (!payload)
			return;

		m_CurrentActorId = payload->actorId;
		UpdateActiveSlotState();
		break;
	}
	case EventType::CombatEnded:
	case EventType::CombatExit:
		m_InCombat = false;
		RemoveUI();
		m_InitiativeOrder.clear();
		m_ActorIcons.clear();
		m_CurrentActorId = 0;
		m_OrderDirty = false;
		break;

	default:
		break;
	}
}

void InitiativeUIComponent::SetEnabled(const bool& enable)
{
	if (m_Enabled == enable)
		return;

	m_Enabled = enable;
	m_InCombat = false;
	m_CurrentActorId = 0;
	m_OrderDirty = false;

	if (!m_Enabled)
	{
		RemoveUI();
	}
}

void InitiativeUIComponent::SetScale(const float& scale)
{
	m_Scale = max(0.0f, scale);
}

void InitiativeUIComponent::RebuildUI()
{
	m_OrderDirty = true;
	RebuildInitiativeOrder();
}

void InitiativeUIComponent::RemoveUI()
{
	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!scene || !uiManager)
	{
		return;
	}

	SetFrameVisible(false);
	ResetIconPools();
}
UIManager* InitiativeUIComponent::GetUIManager() const
{
	auto* scene = GetScene();
	if (!scene)
		return nullptr;

	auto& services = scene->GetServices();
	if (!services.Has<UIManager>())
		return nullptr;

	return &services.Get<UIManager>();
}

Scene* InitiativeUIComponent::GetScene() const
{
	auto* owner = GetOwner();
	return owner ? owner->GetScene() : nullptr;
}

void InitiativeUIComponent::RebuildInitiativeOrder()
{
	if (!m_Enabled)
		return;

	auto* scene     = GetScene();
	auto* uiManager = GetUIManager();
	if (!scene || !uiManager)
		return;

	if (!m_InCombat || m_InitiativeOrder.empty())
	{
		SetFrameVisible(false);
		return;
	}
	
	const std::string& sceneName = scene->GetName();
	auto box   = uiManager->FindUIObject(sceneName, "InitiativeBox");
	auto frame = uiManager->FindUIObject(sceneName, "InitiativeFrame");

	if (!box || !frame)
		return;

	auto* horizontalBox = box->GetComponent<HorizontalBox>();
	if (!horizontalBox)
		return;

	ResetIconPools();
	m_ActorInfo.clear();
	m_ActorIcons.clear();

	int playerIndex = 0;
	int enemyIndex = 0;
	for (int actorId : m_InitiativeOrder)
	{
		const bool isPlayer = actorId == 1;
		const int displayIndex = isPlayer ? ++playerIndex : ++enemyIndex;
		m_ActorInfo[actorId] = ActorIconInfo{ isPlayer, displayIndex };
	}

	horizontalBox->ClearSlots();

	const float iconSize	= 80.0f;
	const float basePadding = 10.0f;

	for (int actorId : m_InitiativeOrder)
	{
		const ActorIconInfo info = ResolveActorInfo(actorId);
		std::shared_ptr<UIObject> icon;
		if(info.isPlayer)
		{
			if (m_PlayerIconPool.empty())
			{
				continue;
			}
			icon = m_PlayerIconPool.front();
		}
		else
		{
			const size_t enemyIndex = static_cast<size_t>(max(0, info.displayIndex - 1));

			if (enemyIndex >= m_EnemyIconPool.size())
			{
				continue;
			}
			icon = m_EnemyIconPool[enemyIndex];
		}

		if (!icon)
		{
			continue;
		}

		icon->SetIsVisible(true);
		UpdateIconVisuals(*icon, actorId, info);
		m_ActorIcons[actorId] = icon;

		HorizontalBoxSlot slot{};
		slot.child		 = icon.get();
		slot.childName   = icon->GetName();
		slot.desiredSize = UISize{ iconSize, iconSize };
		slot.layoutScale = 1.0f;
		slot.padding = UIPadding{ basePadding, basePadding, basePadding, basePadding };
		horizontalBox->AddSlot(slot);
	}

	m_OrderDirty = false;
	uiManager->RefreshUIListForCurrentScene();
	UpdateActiveSlotState();
	SetFrameVisible(true);
}

std::shared_ptr<UIObject> InitiativeUIComponent::FindUI(UIManager& uiManager, Scene& scene, const std::string& name) const
{
	const std::string sceneName = scene.GetName();
	auto found = uiManager.FindUIObject(sceneName, name);

	if (found && !found->GetScene())
	{
		found->SetScene(&scene);
	}

	return found;
}

InitiativeUIComponent::ActorIconInfo InitiativeUIComponent::ResolveActorInfo(int actorId) const
{
	auto it = m_ActorInfo.find(actorId);
	
	if (it != m_ActorInfo.end())
	{
		return it->second;
	}

	return ActorIconInfo{ false, actorId };
}

void InitiativeUIComponent::ResetIconPools()
{
	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!scene || !uiManager)
	{
		return;
	}

	m_PlayerIconPool.clear();
	m_EnemyIconPool.clear();

	for (const auto& name : m_PlayerIconNames)
	{
		if (auto icon = FindUI(*uiManager, *scene, name))
		{
			icon->SetIsVisible(false);
			m_PlayerIconPool.push_back(icon);
		}
	}

	for (const auto& name : m_EnemyIconNames)
	{
		if (auto icon = FindUI(*uiManager, *scene, name))
		{
			icon->SetIsVisible(false);
			m_EnemyIconPool.push_back(icon);
		}
	}
}

void InitiativeUIComponent::UpdateIconVisuals(UIObject& iconObject, int actorId, const ActorIconInfo& info) const
{
	(void)info;

	auto* text = iconObject.GetComponent<UITextComponent>();
	if (text)
	{
		iconObject.RemoveComponentByTypeName(UITextComponent::StaticTypeName, 0);
	}

	auto* image = iconObject.GetComponent<UIImageComponent>();
	if (!image)
	{
		image = iconObject.AddComponent<UIImageComponent>();
	}

	const bool isDead = IsActorDead(actorId);
	if (isDead && m_DeadIconTexture.IsValid())
	{
		image->SetTextureHandle(m_DeadIconTexture);
		return;
	}
}

void InitiativeUIComponent::UpdateActiveSlotState()
{
	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!scene || !uiManager)
	{
		return;
	}

	const std::string sceneName = scene->GetName();
	auto box = uiManager->FindUIObject(sceneName, "InitiativeBox");
	if (!box)
	{
		return;
	}

	auto* horizontalBox = box->GetComponent<HorizontalBox>();
	if (!horizontalBox)
	{
		return;
	}

	auto& slots = horizontalBox->GetSlotsMutable();
	for (auto& slot : slots)
	{
		if (!slot.child)
		{
			continue;
		}

		const auto it = std::find_if(m_ActorIcons.begin(), m_ActorIcons.end(),
			[&slot](const auto& pair)
			{
				return pair.second.get() == slot.child;
			});

		if (it != m_ActorIcons.end())
		{
			slot.layoutScale = it->first == m_CurrentActorId ? m_Scale : 1.0f;
			const ActorIconInfo info = ResolveActorInfo(m_CurrentActorId);
			UpdateIconVisuals(*it->second, m_CurrentActorId, info);
		}
	}
}

void InitiativeUIComponent::SetFrameVisible(bool visible)
{
	auto* scene		= GetScene();
	auto* uiManager = GetUIManager();

	if (!scene || !uiManager)
	{
		return;
	}

	const std::string sceneName = scene->GetName();
	const std::vector<std::string> names = {
		"InitiativeFrame",
		"InitiativeBox"
	};

	for (const auto& name : names)
	{
		auto uiObject = uiManager->FindUIObject(sceneName, name);
		if (uiObject)
		{
			uiObject->SetIsVisible(visible);
		}
	}
}

bool InitiativeUIComponent::IsActorDead(int actorId) const
{
	auto* scene = GetScene();
	if (!scene)
	{
		return false;
	}

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		if (!object)
		{
			continue;
		}

		if (auto* player = object->GetComponent<PlayerComponent>())
		{
			if(player->GetActorId() == actorId)
			{
				if (auto* stat = object->GetComponent<PlayerStatComponent>())
				{
					return stat->IsDead();
				}
			}
		}
	}

	return false;
}