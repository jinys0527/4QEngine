#include "UINumberSpriteComponent.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"
#include "UIObject.h"
#include "UIImageComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "PlayerStatComponent.h"
#include "EnemyStatComponent.h"
#include "GameManager.h"
#include <algorithm>
#include <cmath>

REGISTER_UI_COMPONENT(UINumberSpriteComponent)
REGISTER_PROPERTY(UINumberSpriteComponent, Enabled)
REGISTER_PROPERTY(UINumberSpriteComponent, Value)
REGISTER_PROPERTY(UINumberSpriteComponent, LeadingZero)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitTextures)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitSpacing)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitOffsets)
REGISTER_PROPERTY(UINumberSpriteComponent, PerDigitAdvance)
REGISTER_PROPERTY(UINumberSpriteComponent, FixedDigitCount)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitTintColor)
REGISTER_PROPERTY(UINumberSpriteComponent, AutoValueSource)
REGISTER_PROPERTY(UINumberSpriteComponent, AutoValueActorId)
REGISTER_PROPERTY(UINumberSpriteComponent, UseAsCombatPopup)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupObjectNames)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupTrackActorId)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupRiseDistance)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupLifetime)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupFadeOutTime)
REGISTER_PROPERTY(UINumberSpriteComponent, DamageTint)
REGISTER_PROPERTY(UINumberSpriteComponent, HealTint)
REGISTER_PROPERTY(UINumberSpriteComponent, DealTint)
REGISTER_PROPERTY(UINumberSpriteComponent, CriticalTint)
REGISTER_PROPERTY(UINumberSpriteComponent, GoldTint)

UINumberSpriteComponent::~UINumberSpriteComponent()
{
	if (m_ListenerRegistered && m_Dispatcher && m_Dispatcher->IsAlive())
	{
		if (m_Dispatcher->FindListeners(EventType::CombatNumberPopup))
		{
			m_Dispatcher->RemoveListener(EventType::CombatNumberPopup, this);
		}
		if (m_Dispatcher->FindListeners(EventType::GoldAcquired))
		{
			m_Dispatcher->RemoveListener(EventType::GoldAcquired, this);
		}
	}
}

void UINumberSpriteComponent::Start()
{
	m_RuntimeBindingsReady = false;
	m_ListenerRegistered = false;
	m_PopupPoolDirty = true;
	m_ValueDirty = true;
	m_DigitTargets.clear();
	m_BaseDigitBounds.clear();
}

void UINumberSpriteComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
	(void)deltaTime;

	if (!m_Enabled)
	{
		return;
	}

	if (!TryPrepareRuntimeBindings())
	{
		return;
	}

	if (m_DigitTargets.empty())
	{
		EnsureDigitTargetsResolved();
	}
	TryInitializePopupPool();

	TickPopups(deltaTime);
	TryUpdateValueFromAutoSource();

	if (!m_ValueDirty)
	{
		return;
	}

	ApplyValue();
	m_ValueDirty = false;
}

void UINumberSpriteComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);

	if (!m_Enabled)
	{
		return;
	}

	if (m_UseAsCombatPopup && type == EventType::GoldAcquired && data)
	{
		const auto* payload = static_cast<const Events::GoldAcquiredEvent*>(data);
		if (payload && payload->amount > 0)
		{
			ShowPopup(payload->amount, m_GoldTint);
		}
		return;
	}

	if (!m_UseAsCombatPopup || type != EventType::CombatNumberPopup || !data)
	{
		return;
	}

	const auto* payload = static_cast<const Events::CombatNumberPopupEvent*>(data);
	if (!payload)
	{
		return;
	}

	if (m_PopupTrackActorId != 0 && payload->targetActorId != m_PopupTrackActorId)
	{
		return;
	}

	const int value = std::abs(payload->hpDelta);
	if (!payload->isMiss && value <= 0)
	{
		return;
	}

	DirectX::XMFLOAT4 tint = m_DamageTint;
	if (payload->isCritical)
	{
		tint = m_CriticalTint;
	}
	else if (payload->hpDelta > 0)
	{
		tint = m_HealTint;
	}
	else if (payload->instigatorActorId == m_PopupTrackActorId)
	{
		tint = m_DealTint;
	}

	ShowPopup(payload->isMiss ? 0 : value, tint);
}

void UINumberSpriteComponent::SetEnabled(const bool& enabled)
{
	if (m_Enabled == enabled)
	{
		return;
	}

	m_Enabled = enabled;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetValue(const int& value)
{
	const int clamped = max(0, value);
	if (m_Value == clamped)
	{
		return;
	}

	m_Value = clamped;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetLeadingZero(const bool& leadingZero)
{
	if (m_LeadingZero == leadingZero)
	{
		return;
	}

	m_LeadingZero = leadingZero;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitTextureHandle(int digit, const TextureHandle& handle)
{
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return;
	}

	m_DigitTextures[static_cast<size_t>(digit)] = handle;
	m_ValueDirty = true;
}

const TextureHandle& UINumberSpriteComponent::GetDigitTextureHandle(int digit) const
{
	static TextureHandle invalid = TextureHandle::Invalid();
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return invalid;
	}
	return m_DigitTextures[static_cast<size_t>(digit)];
}

void UINumberSpriteComponent::SetDigitTextures(const std::array<TextureHandle, 10>& textures)
{
	m_DigitTextures = textures;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitSpacing(const float& spacing)
{
	m_DigitSpacing = spacing;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitOffsets(std::vector<UIAnchor> offsets)
{
	m_DigitOffsets = std::move(offsets);
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetPerDigitAdvance(const std::array<float, 10>& offsets)
{
	m_PerDigitAdvance = offsets;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetFixedDigitCount(const int& count)
{
	m_FixedDigitCount = (std::max)(0, count);
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitTintColor(const DirectX::XMFLOAT4& color)
{
	m_DigitTintColor = color;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetAutoValueSource(const int& source)
{
	m_AutoValueSource = source;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetAutoValueActorId(const int& actorId)
{
	m_AutoValueActorId = actorId;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetUseAsCombatPopup(const bool& useAsPopup)
{
	m_UseAsCombatPopup = useAsPopup;
}

void UINumberSpriteComponent::SetPopupObjectNames(std::vector<std::string> names)
{
	m_PopupObjectNames = std::move(names);
	m_PopupPoolDirty = true;
}

void UINumberSpriteComponent::SetPopupTrackActorId(const int& actorId)
{
	m_PopupTrackActorId = actorId;
}

void UINumberSpriteComponent::SetPopupRiseDistance(const float& rise)
{
	m_PopupRiseDistance = rise;
}

void UINumberSpriteComponent::SetPopupLifetime(const float& time)
{
	m_PopupLifetime = (std::max)(0.05f, time);
}

void UINumberSpriteComponent::SetPopupFadeOutTime(const float& time)
{
	m_PopupFadeOutTime = (std::max)(0.01f, time);
}

void UINumberSpriteComponent::SetDamageTint(const DirectX::XMFLOAT4& color)
{
	m_DamageTint = color;
}

void UINumberSpriteComponent::SetHealTint(const DirectX::XMFLOAT4& color)
{
	m_HealTint = color;
}

void UINumberSpriteComponent::SetDealTint(const DirectX::XMFLOAT4& color)
{
	m_DealTint = color;
}

void UINumberSpriteComponent::SetCriticalTint(const DirectX::XMFLOAT4& color)
{
	m_CriticalTint = color;
}

void UINumberSpriteComponent::SetGoldTint(const DirectX::XMFLOAT4& color)
{
	m_GoldTint = color;
}

void UINumberSpriteComponent::RefreshVisuals()
{
	m_ValueDirty = true;
}

UIObject* UINumberSpriteComponent::FindUIObject(const std::string& name) const
{
	if (name.empty())
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

bool UINumberSpriteComponent::TryUpdateValueFromAutoSource()
{
	if (m_AutoValueSource == 0)
	{
		return false;
	}

	auto* scene = GetScene();
	if (!scene)
	{
		return false;
	}

	auto resolvePlayer = [scene]() -> PlayerComponent*
		{
			for (const auto& [name, object] : scene->GetGameObjects())
			{
				(void)name;
				if (!object)
				{
					continue;
				}
				if (auto* player = object->GetComponent<PlayerComponent>())
				{
					return player;
				}
			}
			return nullptr;
		};

	auto resolveEnemyStat = [scene, this]() -> EnemyStatComponent*
		{
			if (m_AutoValueActorId == 0)
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
				auto* enemy = object->GetComponent<EnemyComponent>();
				if (!enemy || enemy->GetActorId() != m_AutoValueActorId)
				{
					continue;
				}
				return object->GetComponent<EnemyStatComponent>();
			}
			return nullptr;
		};

	int nextValue = m_Value;
	bool valid = false;
	switch (m_AutoValueSource)
	{
	case 1: // Player current HP
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetCurrentHP();
				valid = true;
			}
		}
		break;
	}
	case 2: // Player max HP
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				int floor = 1;
				if (scene->GetServices().Has<GameManager>())
				{
					floor = scene->GetServices().Get<GameManager>().GetCurrentFloor();
				}
				nextValue = stat->GetMaxHealthForFloor((std::max)(1, floor));
				valid = true;
			}
		}
		break;
	}
	case 3: // Enemy current HP
	{
		if (auto* stat = resolveEnemyStat())
		{
			nextValue = stat->GetCurrentHP();
			valid = true;
		}
		break;
	}
	case 4: // Enemy max HP
	{
		if (auto* stat = resolveEnemyStat())
		{
			nextValue = stat->GetInitialHP();
			valid = true;
		}
		break;
	}
	case 5: // Player gold
	{
		if (auto* player = resolvePlayer())
		{
			nextValue = player->GetMoney();
			valid = true;
		}
		break;
	}
	default:
		break;
	}

	if (!valid)
	{
		return false;
	}

	SetValue(nextValue);
	return true;
}

bool UINumberSpriteComponent::EnsureDigitTargetsResolved()
{
	std::vector<UIObject*> resolvedTargets;

	auto* owner = dynamic_cast<UIObject*>(GetOwner());
	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!owner || !scene || !uiManager)
	{
		return !m_DigitTargets.empty();
	}

	const std::string& ownerName = owner->GetName();
	if (ownerName.empty())
	{
		return !m_DigitTargets.empty();
	}

	auto& allScenes = uiManager->GetUIObjects();
	auto sceneIt = allScenes.find(scene->GetName());
	if (sceneIt == allScenes.end())
	{
		return !m_DigitTargets.empty();
	}

	auto& sceneObjects = sceneIt->second;
	std::vector<std::pair<std::string, UIObject*>> candidates;
	candidates.reserve(sceneObjects.size());
	for (const auto& [name, object] : sceneObjects)
	{
		if (!object || object->GetParentName() != ownerName)
		{
			continue;
		}

		auto* target = object.get();
		if (!target->GetComponent<UIImageComponent>())
		{
			continue;
		}

		candidates.emplace_back(name, target);
	}

	std::sort(candidates.begin(), candidates.end(),
		[](const auto& lhs, const auto& rhs)
		{
			const int lhsZ = lhs.second ? lhs.second->GetZOrder() : 0;
			const int rhsZ = rhs.second ? rhs.second->GetZOrder() : 0;
			if (lhsZ != rhsZ)
			{
				return lhsZ < rhsZ;
			}

			return lhs.first < rhs.first;
		});

	resolvedTargets.reserve(candidates.size());
	for (const auto& [name, target] : candidates)
	{
		(void)name;
		resolvedTargets.push_back(target);
	}

	if (m_DigitTargets != resolvedTargets)
	{
		m_DigitTargets = std::move(resolvedTargets);
		m_BaseDigitBounds.clear();
		m_ValueDirty = true;
	}

	return !m_DigitTargets.empty();
}

void UINumberSpriteComponent::ApplyValue()
{
	if (m_DigitTargets.empty())
	{
		return;
	}

	if (m_BaseDigitBounds.size() != m_DigitTargets.size())
	{
		m_BaseDigitBounds.resize(m_DigitTargets.size());
		for (size_t i = 0; i < m_DigitTargets.size(); ++i)
		{
			auto* target = m_DigitTargets[i];
			if (target && target->HasBounds())
			{
				m_BaseDigitBounds[i] = target->GetBounds();
			}
		}
	}

	std::string valueText = std::to_string(m_Value);
	const int configuredDigits = (m_FixedDigitCount > 0) ? m_FixedDigitCount : static_cast<int>(m_DigitTargets.size());
	if (static_cast<int>(valueText.size()) < configuredDigits)
	{
		valueText = std::string(static_cast<size_t>(configuredDigits - valueText.size()), '0') + valueText;
	}


	const int digitCount = static_cast<int>(valueText.size());
	const int slotCount = static_cast<int>(m_DigitTargets.size());
	const int leadingSlots = max(0, slotCount - digitCount);
	float accumulatedOffset = 0.0f;

	for (int index = 0; index < slotCount; ++index)
	{
		auto* target = m_DigitTargets[static_cast<size_t>(index)];

		if (!target)
		{
			continue;
		}

		const bool isLeadingSlot = index < leadingSlots;
		if (isLeadingSlot && !m_LeadingZero)
		{
			target->SetIsVisibleFromComponent(false);
			continue;
		}

		const int valueIndex = isLeadingSlot ? 0 : index - leadingSlots;
		const char digitChar = valueText[static_cast<size_t>(valueIndex)];
		const int digitValue = std::clamp(static_cast<int>(digitChar - '0'), 0, 9);

		target->SetIsVisibleFromComponent(true);
		if (auto* image = target->GetComponent<UIImageComponent>())
		{
			const auto& handle = m_DigitTextures[static_cast<size_t>(digitValue)];
			if (handle.IsValid())
			{
				image->SetTextureHandle(handle);
			}
			image->SetTintColor(m_DigitTintColor);
		}

		if (target->HasBounds() && index < static_cast<int>(m_BaseDigitBounds.size()))
		{
			UIRect adjusted = m_BaseDigitBounds[static_cast<size_t>(index)];
			adjusted.x += accumulatedOffset;
			if (index < static_cast<int>(m_DigitOffsets.size()))
			{
				adjusted.x += m_DigitOffsets[static_cast<size_t>(index)].x;
				adjusted.y += m_DigitOffsets[static_cast<size_t>(index)].y;
			}
			target->SetBounds(adjusted);
		}

		accumulatedOffset += m_DigitSpacing + m_PerDigitAdvance[static_cast<size_t>(digitValue)];
	}
}

bool UINumberSpriteComponent::TryPrepareRuntimeBindings()
{
	if (m_RuntimeBindingsReady)
	{
		return true;
	}

	auto* scene = GetScene();
	if (!scene)
	{
		return false;
	}

	auto* uiManager = GetUIManager();
	if (!uiManager)
	{
		return false;
	}

	if (!ArePopupTargetsReady())
	{
		return false;
	}

	m_UIManager = uiManager;
	m_Dispatcher = &GetEventDispatcher();
	if (!m_ListenerRegistered)
	{
		m_Dispatcher->AddListener(EventType::CombatNumberPopup, this);
		m_Dispatcher->AddListener(EventType::GoldAcquired, this);
		m_ListenerRegistered = true;
	}

	m_RuntimeBindingsReady = true;
	m_PopupPoolDirty = true;
	return true;
}

bool UINumberSpriteComponent::ArePopupTargetsReady() const
{
	if (m_PopupObjectNames.empty())
	{
		return true;
	}

	for (const auto& name : m_PopupObjectNames)
	{
		if (name.empty())
		{
			continue;
		}

		if (!FindUIObject(name))
		{
			return false;
		}
	}

	return true;
}

void UINumberSpriteComponent::TryInitializePopupPool()
{
	if (!m_PopupPoolDirty)
	{
		return;
	}

	auto* uiManager = GetUIManager();
	if (!uiManager)
	{
		return;
	}

	UpdatePopupPool();
	m_PopupPoolDirty = false;
}


void UINumberSpriteComponent::UpdatePopupPool()
{
	m_PopupStates.clear();
	m_PopupStates.reserve(m_PopupObjectNames.size());
	for (const auto& name : m_PopupObjectNames)
	{
		PopupState state{};
		state.objectName = name;
		if (auto* popup = FindUIObject(name))
		{
			popup->SetIsVisibleFromComponent(false);
			popup->SetOpacityFromComponent(1.0f);
			if (popup->HasBounds())
			{
				state.baseBounds = popup->GetBounds();
			}
		}
		m_PopupStates.push_back(state);
	}
}

void UINumberSpriteComponent::TickPopups(float deltaTime)
{
	for (auto& popup : m_PopupStates)
	{
		if (!popup.active)
		{
			continue;
		}

		auto* target = FindUIObject(popup.objectName);
		if (!target)
		{
			popup.active = false;
			continue;
		}

		popup.elapsed += deltaTime;
		const float t = min(1.0f, popup.elapsed / max(0.01f, m_PopupLifetime));
		UIRect moved = popup.baseBounds;
		moved.y -= m_PopupRiseDistance * t;
		target->SetBounds(moved);

		float opacity = 1.0f;
		const float fadeStart = max(0.0f, m_PopupLifetime - m_PopupFadeOutTime);
		if (popup.elapsed > fadeStart)
		{
			const float fadeT = (popup.elapsed - fadeStart) / max(0.01f, m_PopupFadeOutTime);
			opacity = max(0.0f, 1.0f - fadeT);
		}
		target->SetOpacityFromComponent(opacity);

		if (popup.elapsed >= m_PopupLifetime)
		{
			popup.active = false;
			target->SetIsVisibleFromComponent(false);
			target->SetOpacityFromComponent(1.0f);
			target->SetBounds(popup.baseBounds);
		}
	}
}

void UINumberSpriteComponent::ShowPopup(int value, const DirectX::XMFLOAT4& tint)
{
	TryInitializePopupPool();

	auto pick = std::find_if(m_PopupStates.begin(), m_PopupStates.end(), [](const PopupState& state) { return !state.active; });
	if (pick == m_PopupStates.end() && !m_PopupStates.empty())
	{
		pick = std::min_element(m_PopupStates.begin(), m_PopupStates.end(), [](const PopupState& a, const PopupState& b)
			{
				return a.elapsed > b.elapsed;
			});
	}

	if (pick == m_PopupStates.end())
	{
		return;
	}

	auto* target = FindUIObject(pick->objectName);
	if (!target)
	{
		return;
	}

	if (target->HasBounds())
	{
		pick->baseBounds = target->GetBounds();
	}

	if (auto* popupNumber = target->GetComponent<UINumberSpriteComponent>())
	{
		popupNumber->SetDigitTintColor(tint);
		popupNumber->SetValue(value);
		popupNumber->RefreshVisuals();
	}

	target->SetBounds(pick->baseBounds);
	target->SetOpacityFromComponent(1.0f);
	target->SetIsVisibleFromComponent(true);
	pick->elapsed = 0.0f;
	pick->active = true;
}