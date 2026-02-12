#include "TurnTimerUIComponent.h"

#include "Event.h"
#include "EventDispatcher.h"
#include "GameManager.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "UINumberSpriteComponent.h"
#include "UIProgressBarComponent.h"
#include <algorithm>
#include <cmath>

REGISTER_UI_COMPONENT(TurnTimerUIComponent)
REGISTER_PROPERTY(TurnTimerUIComponent, Enabled)
REGISTER_PROPERTY(TurnTimerUIComponent, UpdateNumberSprite)
REGISTER_PROPERTY(TurnTimerUIComponent, HideWhenInactive)
REGISTER_PROPERTY(TurnTimerUIComponent, RoundUpSeconds)

TurnTimerUIComponent::~TurnTimerUIComponent()
{
	DetachFromDispatcher();
}

void TurnTimerUIComponent::Start()
{
	m_Dispatcher = nullptr;
	m_ListenerRegistered = false;
	m_RuntimeBindingsReady = false;
}

void TurnTimerUIComponent::Update(float deltaTime)
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

	auto* scene = GetScene();
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (!gameManager)
	{
		return;
	}

	const bool isPlayerTurnActive = gameManager->GetTurn() == Turn::PlayerTurn
		&& ((gameManager->GetPhase() == Phase::ExplorationLoop && gameManager->GetExplorationTurnState() == ExplorationTurnState::PlayerTurn)
			|| (gameManager->GetPhase() == Phase::TurnBasedCombat && gameManager->GetCombatTurnState() == CombatTurnState::PlayerTurn));

	ApplyTimerVisuals(
		gameManager->GetPlayerTurnRemainingSeconds(),
		gameManager->GetPlayerTurnRemainingRatio(),
		isPlayerTurnActive);
}

void TurnTimerUIComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);

	if (!m_Enabled || type != EventType::PlayerTurnTimerChanged || !data)
	{
		return;
	}

	const auto* payload = static_cast<const Events::PlayerTurnTimerChangedEvent*>(data);
	if (!payload)
	{
		return;
	}

	ApplyTimerVisuals(payload->remainingSeconds, payload->remainingRatio, payload->isPlayerTurnActive);
}

void TurnTimerUIComponent::SetEnabled(const bool& enabled)
{
	m_Enabled = enabled;
}

void TurnTimerUIComponent::SetUpdateNumberSprite(const bool& enabled)
{
	m_UpdateNumberSprite = enabled;
}

void TurnTimerUIComponent::SetHideWhenInactive(const bool& enabled)
{
	m_HideWhenInactive = enabled;
}

void TurnTimerUIComponent::SetRoundUpSeconds(const bool& enabled)
{
	m_RoundUpSeconds = enabled;
}

bool TurnTimerUIComponent::TryPrepareRuntimeBindings()
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

	m_Dispatcher = &GetEventDispatcher();
	if (!m_ListenerRegistered)
	{
		m_Dispatcher->AddListener(EventType::PlayerTurnTimerChanged, this);
		m_ListenerRegistered = true;
	}

	m_RuntimeBindingsReady = true;
	return true;
}

void TurnTimerUIComponent::DetachFromDispatcher()
{
	if (!m_ListenerRegistered || !m_Dispatcher || !m_Dispatcher->IsAlive())
	{
		return;
	}

	if (m_Dispatcher->FindListeners(EventType::PlayerTurnTimerChanged))
	{
		m_Dispatcher->RemoveListener(EventType::PlayerTurnTimerChanged, this);
	}

	m_ListenerRegistered = false;
}

void TurnTimerUIComponent::ApplyTimerVisuals(float remainingSeconds, float remainingRatio, bool isPlayerTurnActive)
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* progress = owner->GetComponent<UIProgressBarComponent>();
	auto* number = m_UpdateNumberSprite ? owner->GetComponent<UINumberSpriteComponent>() : nullptr;

	if (progress)
	{
		progress->SetPercent(std::clamp(remainingRatio, 0.0f, 1.0f));
	}

	if (m_UpdateNumberSprite && number)
	{
		const float clampedSeconds = (std::max)(0.0f, remainingSeconds);
		const int displaySeconds = m_RoundUpSeconds
			? static_cast<int>(std::ceil(clampedSeconds))
			: static_cast<int>(std::floor(clampedSeconds));
		number->SetValue(displaySeconds);
	}

	if (m_HideWhenInactive)
	{
		SetVisible(isPlayerTurnActive);
	}
}