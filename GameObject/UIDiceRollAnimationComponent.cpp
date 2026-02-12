#include "UIDiceRollAnimationComponent.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"
#include "UIObject.h"
#include <algorithm>
#include <iostream>
#include <random>

REGISTER_UI_COMPONENT(UIDiceRollAnimationComponent)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, Enabled)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, DiceContext)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, DelayMin)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, DelayMax)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, AnimationDuration)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, ScaleStart)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, ScalePeak)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, ScaleEnd)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, ApplyToDigits)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, AnimateIndividuals)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, TensDigitObjectName)
REGISTER_PROPERTY(UIDiceRollAnimationComponent, OnesDigitObjectName)

namespace
{
	std::string ResolveOwnerName(const UIDiceRollAnimationComponent* component)
	{
		if (!component)
		{
			return std::string{};
		}

		auto* owner = component->GetOwner();
		return owner ? owner->GetName() : std::string{};
	}

	bool IsOwnerVisible(const UIDiceRollAnimationComponent* component)
	{
		if (!component)
		{
			return false;
		}

		auto* owner = dynamic_cast<UIObject*>(component->GetOwner());
		return owner && owner->IsVisible();
	}
}

UIDiceRollAnimationComponent::~UIDiceRollAnimationComponent()
{
	if (m_ListenerRegistered && m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::DiceRolled))
	{
		m_Dispatcher->RemoveListener(EventType::DiceRolled, this);
	}
}

void UIDiceRollAnimationComponent::Start()
{
	m_RuntimeBindingsReady = false;
	m_ListenerRegistered = false;
}

void UIDiceRollAnimationComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);

	// 비활성 슬롯도 DiceRolled 리스너는 미리 준비되어야
	// 활성화 직후(같은 프레임) 들어오는 롤 이벤트를 놓치지 않는다.
	if (!TryPrepareRuntimeBindings())
	{
		return;
	}

	if (!m_Enabled)
	{
		return;
	}

	if (m_Waiting)
	{
		m_DelayTimer -= deltaTime;
		if (m_DelayTimer <= 0.0f)
		{
			m_Waiting = false;
			m_Animating = true;
			m_AnimationTimer = 0.0f;
		}
	}

	if (!m_Animating)
	{
		return;
	}

	m_AnimationTimer += deltaTime;
	const float duration = max(0.001f, m_AnimationDuration);
	const float t = std::clamp(m_AnimationTimer / duration, 0.0f, 1.0f);
	ApplyScale(EvaluateScale(t));

	if (m_AnimationTimer >= duration)
	{
		RestoreBounds();
		m_Animating = false;
		if (m_Dispatcher)
		{
			if (IsOwnerVisible(this))
			{
				std::cout << "[UIDiceAnim] completed owner=" << ResolveOwnerName(this)
					<< " slotContext=" << m_DiceContext << std::endl;
			}
			m_Dispatcher->Dispatch(EventType::PlayerDiceAnimationCompleted, nullptr);
		}
	}
}

void UIDiceRollAnimationComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);

	if (!m_Enabled || type != EventType::DiceRolled)
	{
		return;
	}

	if (!TryPrepareRuntimeBindings())
	{
		return;
	}

	const auto* payload = static_cast<const Events::DiceRollEvent*>(data);
	if (!payload)
	{
		return;
	}

	const std::string ownerName = ResolveOwnerName(this);

	if (!m_DiceContext.empty() && payload->context != m_DiceContext)
	{
		// 기본 규칙:
	// - total 이벤트는 정확히 동일 컨텍스트에만 적용
	// - individual 이벤트는 "<slotContext>_<idx>" 자식 컨텍스트도 허용
	//   (예: slot=InitiativeStatRoll_2, event=InitiativeStatRoll_2_1)
		const std::string childPrefix = m_DiceContext + "_";
		const bool isChildContext = payload->context.rfind(childPrefix, 0) == 0;
		if (payload->isTotal || !isChildContext)
		{
			return;
		}
	}

	if (!payload->isTotal && !m_AnimateIndividuals)
	{
		return;
	}

	if (IsOwnerVisible(this))
	{
		std::cout << "[UIDiceAnim] trigger owner=" << ownerName
			<< " slotContext=" << m_DiceContext
			<< " eventContext=" << payload->context
			<< " isTotal=" << payload->isTotal << std::endl;
	}
	BeginAnimation();
}

bool UIDiceRollAnimationComponent::TryPrepareRuntimeBindings()
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

	m_UIManager = uiManager;
	m_Dispatcher = &GetEventDispatcher();
	if (!m_ListenerRegistered)
	{
		m_Dispatcher->AddListener(EventType::DiceRolled, this);
		m_ListenerRegistered = true;
	}

	m_RuntimeBindingsReady = true;
	return true;
}

void UIDiceRollAnimationComponent::SetEnabled(const bool& enabled)
{
	if (m_Enabled == enabled)
	{
		return;
	}

	m_Enabled = enabled;
	if (m_Enabled)
	{
		// 활성화 직후 수신 누락 방지를 위해 즉시 바인딩 시도.
		TryPrepareRuntimeBindings();
		return;
	}

	const bool wasActive = m_Waiting || m_Animating;
	m_Waiting = false;
	m_Animating = false;
	m_DelayTimer = 0.0f;
	m_AnimationTimer = 0.0f;
	RestoreBounds();

	// 활성 상태에서 비활성화되면 완료 이벤트를 보정 발행해
	// 상위 FSM/매니저의 애니메이션 카운터가 고정되지 않게 한다.
	if (wasActive && m_Dispatcher)
	{
		if (IsOwnerVisible(this))
		{
			std::cout << "[UIDiceAnim] cancel->complete owner=" << ResolveOwnerName(this)
				<< " slotContext=" << m_DiceContext << std::endl;
		}
		m_Dispatcher->Dispatch(EventType::PlayerDiceAnimationCompleted, nullptr);
	}
}

void UIDiceRollAnimationComponent::SetDiceContext(const std::string& context)
{
	m_DiceContext = context;
}

void UIDiceRollAnimationComponent::SetDelayRange(float minDelay, float maxDelay)
{
	m_DelayMin = min(minDelay, maxDelay);
	m_DelayMax = max(minDelay, maxDelay);
}

void UIDiceRollAnimationComponent::SetDelayMin(const float& value)
{
	m_DelayMin = value;
	if (m_DelayMax < m_DelayMin)
	{
		m_DelayMax = m_DelayMin;
	}
}

void UIDiceRollAnimationComponent::SetDelayMax(const float& value)
{
	m_DelayMax = value;
	if (m_DelayMin > m_DelayMax)
	{
		m_DelayMin = m_DelayMax;
	}
}

void UIDiceRollAnimationComponent::SetAnimationDuration(const float& duration)
{
	m_AnimationDuration = max(0.0f, duration);
}

void UIDiceRollAnimationComponent::SetScaleStart(const float& scale)
{
	m_ScaleStart = scale;
}

void UIDiceRollAnimationComponent::SetScalePeak(const float& scale)
{
	m_ScalePeak = scale;
}

void UIDiceRollAnimationComponent::SetScaleEnd(const float& scale)
{
	m_ScaleEnd = scale;
}

void UIDiceRollAnimationComponent::SetApplyToDigits(const bool& apply)
{
	m_ApplyToDigits = apply;
}

void UIDiceRollAnimationComponent::SetAnimateIndividuals(const bool& animate)
{
	m_AnimateIndividuals = animate;
}

void UIDiceRollAnimationComponent::SetTensDigitObjectName(const std::string& name)
{
	m_TensDigitObjectName = name;
}

void UIDiceRollAnimationComponent::SetOnesDigitObjectName(const std::string& name)
{
	m_OnesDigitObjectName = name;
}

UIObject* UIDiceRollAnimationComponent::FindUIObject(const std::string& name) const
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



void UIDiceRollAnimationComponent::BeginAnimation()
{
	// 동일 슬롯에 대한 연속 DiceRolled 이벤트가 한 프레임 내 다수 들어오더라도
	// start/completed 카운트 불일치가 생기지 않도록, 진행 중이면 중복 시작을 막는다.
	if (m_Waiting || m_Animating)
	{
		if (IsOwnerVisible(this))
		{
			std::cout << "[UIDiceAnim] ignore duplicate start owner=" << ResolveOwnerName(this)
				<< " slotContext=" << m_DiceContext
				<< " waiting=" << m_Waiting
				<< " animating=" << m_Animating << std::endl;
		}
		return;
	}

	CacheBounds();
	m_DelayTimer = GetRandomDelay();
	m_Waiting = true;
	m_Animating = false;
	if (m_Dispatcher)
	{
		if (IsOwnerVisible(this))
		{
			std::cout << "[UIDiceAnim] started owner=" << ResolveOwnerName(this)
				<< " slotContext=" << m_DiceContext
				<< " delay=" << m_DelayTimer
				<< " duration=" << m_AnimationDuration << std::endl;
		}
		m_Dispatcher->Dispatch(EventType::PlayerDiceAnimationStarted, nullptr);
	}
}

void UIDiceRollAnimationComponent::CacheBounds()
{
	m_Targets.clear();

	if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
	{
		if (owner->HasBounds())
		{
			m_Targets.push_back({ owner, owner->GetBounds() });
		}
	}

	if (m_ApplyToDigits)
	{
		if (auto* tens = FindUIObject(m_TensDigitObjectName))
		{
			if (tens->HasBounds())
			{
				m_Targets.push_back({ tens, tens->GetBounds() });
			}
		}

		if (auto* ones = FindUIObject(m_OnesDigitObjectName))
		{
			if (ones->HasBounds())
			{
				m_Targets.push_back({ ones, ones->GetBounds() });
			}
		}
	}
}

void UIDiceRollAnimationComponent::ApplyScale(float scale)
{
	for (const auto& target : m_Targets)
	{
		if (!target.object)
		{
			continue;
		}

		const float centerX = target.bounds.x + target.bounds.width * 0.5f;
		const float centerY = target.bounds.y + target.bounds.height * 0.5f;
		const float width = target.bounds.width * scale;
		const float height = target.bounds.height * scale;
		UIRect scaledBounds = target.bounds;
		scaledBounds.width = width;
		scaledBounds.height = height;
		scaledBounds.x = centerX - width * 0.5f;
		scaledBounds.y = centerY - height * 0.5f;
		target.object->SetBounds(scaledBounds);
	}
}

void UIDiceRollAnimationComponent::RestoreBounds()
{
	for (const auto& target : m_Targets)
	{
		if (target.object)
		{
			target.object->SetBounds(target.bounds);
		}
	}
}

float UIDiceRollAnimationComponent::GetRandomDelay()
{
	if (m_DelayMax <= 0.0f || m_DelayMax <= m_DelayMin)
	{
		return max(0.0f, m_DelayMax);
	}

	static thread_local std::mt19937 generator{ std::random_device{}() };
	std::uniform_real_distribution<float> dist(m_DelayMin, m_DelayMax);
	return dist(generator);
}

float UIDiceRollAnimationComponent::EvaluateScale(float t) const
{
	if (t <= 0.5f)
	{
		const float localT = t / 0.5f;
		return m_ScaleStart + (m_ScalePeak - m_ScaleStart) * localT;
	}

	const float localT = (t - 0.5f) / 0.5f;
	return m_ScalePeak + (m_ScaleEnd - m_ScalePeak) * localT;
}