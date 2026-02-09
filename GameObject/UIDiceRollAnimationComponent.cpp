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

UIDiceRollAnimationComponent::~UIDiceRollAnimationComponent()
{
	if (m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::DiceRolled))
	{
		m_Dispatcher->RemoveListener(EventType::DiceRolled, this);
	}
}

void UIDiceRollAnimationComponent::Start()
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
	m_Dispatcher->AddListener(EventType::DiceRolled, this);
}

void UIDiceRollAnimationComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);

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
	}
}

void UIDiceRollAnimationComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);

	if (!m_Enabled || type != EventType::DiceRolled)
	{
		return;
	}

	const auto* payload = static_cast<const Events::DiceRollEvent*>(data);
	if (!payload)
	{
		return;
	}

	if (!m_DiceContext.empty() && payload->context != m_DiceContext)
	{
		std::cout << "[UIDiceAnim] skip context mismatch slot=" << m_DiceContext
			<< " payload=" << payload->context << std::endl;
		return;
	}

	if (!payload->isTotal && !m_AnimateIndividuals)
	{
		std::cout << "[UIDiceAnim] skip individual roll. slot=" << m_DiceContext
			<< " payloadContext=" << payload->context << std::endl;
		return;
	}

	std::cout << "[UIDiceAnim] begin animation context=" << payload->context
		<< " value=" << payload->value
		<< " isTotal=" << payload->isTotal << std::endl;

	BeginAnimation();
}

void UIDiceRollAnimationComponent::SetEnabled(const bool& enabled)
{
	m_Enabled = enabled;
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

UIManager* UIDiceRollAnimationComponent::GetUIManager() const
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

Scene* UIDiceRollAnimationComponent::GetScene() const
{
	auto* owner = GetOwner();
	return owner ? owner->GetScene() : nullptr;
}

UIObject* UIDiceRollAnimationComponent::FindUIObject(const std::string& name) const
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

void UIDiceRollAnimationComponent::BeginAnimation()
{
	CacheBounds();
	m_DelayTimer = GetRandomDelay();
	m_Waiting    = true;
	m_Animating  = false;
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