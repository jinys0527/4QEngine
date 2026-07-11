#include "PlayerDoorFSMComponent.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "DoorComponent.h"
#include "ReflectionMacro.h"
#include "PlayerFSMComponent.h"
#include "Object.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "SoundManager.h"
#include "DiceSystem.h"
#include "Event.h"

REGISTER_COMPONENT_DERIVED(PlayerDoorFSMComponent, FSMComponent)

namespace
{
	constexpr int DoorCost = 1;
	constexpr int DoorRollThreshold = 7; // 문 성공 값(이상)
}


PlayerDoorFSMComponent::~PlayerDoorFSMComponent()
{
	if (!m_ListenersRegistered)
	{
		return;
	}

	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceAnimationStarted))
	{
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceAnimationStarted, this);
	}
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::PlayerDiceAnimationCompleted))
	{
		GetEventDispatcher().RemoveListener(EventType::PlayerDiceAnimationCompleted, this);
	}
	if (GetEventDispatcher().IsAlive() && GetEventDispatcher().FindListeners(EventType::TurnChanged))
	{
		GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
	}
}

PlayerDoorFSMComponent::PlayerDoorFSMComponent()
{
	BindActionHandler("Door_ConsumeActResource", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				std::cout << "[DoorFSM][Trace] Door_ConsumeActResource failed: missing PlayerComponent" << std::endl;
				DispatchEvent("Door_Revoke");
				GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
				return;
			}

			const bool consumed = player->ConsumeActResource(DoorCost);
			std::cout << "[DoorFSM][Trace] Door_ConsumeActResource consumed=" << consumed
				<< " cost=" << DoorCost << std::endl;
			DispatchEvent(consumed ? "Door_CostPaid" : "Door_Revoke");
		});

	BindActionHandler("Door_Attempt", [this](const FSMAction& action)
		{
			// 난이도 표시 UI
			// 주사위
			std::cout << "[DoorFSM][Trace] Door_Attempt start" << std::endl;
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;

			if (!player)
			{
				std::cout << "[DoorFSM][Trace] Door_Attempt failed: missing PlayerComponent" << std::endl;
				DispatchEvent("Door_Revoke");
				GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
				return;
			}

			bool hasRolled = false;

			if (playerStat)
			{
				auto* scene = owner ? owner->GetScene() : nullptr;
				if (scene)
				{
					auto& services = scene->GetServices();

					std::cout << "[DoorFSM][Trace] Door_Attempt dispatch PlayerDiceRoll" << std::endl;
					GetEventDispatcher().Dispatch(EventType::PlayerDiceRoll, nullptr);

					// 주사위 판정
					if (services.Has<DiceSystem>())
					{
						auto& diceSystem = services.Get<DiceSystem>();
						int bonus = playerStat->GetCalculatedSkillModifier();
						const DiceConfig rollConfig{ 1, 20, bonus };

						const auto roll = diceSystem.Roll(rollConfig, RandomDomain::World);		// Door UI는 슬롯 설정에 따라 total/individual 중 하나만 표시하도록 구성될 수 있어
						std::cout << "[DoorFSM][Trace] Door_Attempt rolled total=" << roll.total
							<< " faceCount=" << roll.faces.size()
							<< " bonus=" << bonus << std::endl;
						// 1d20 결과를 두 형태 모두 브로드캐스트한다.
						const int rolledFace = !roll.faces.empty() ? roll.faces.front() : roll.total;
						const Events::DiceRollEvent individualEvent{ rolledFace, rollConfig.count, rollConfig.sides, rollConfig.bonus, "DoorRoll", false, { rolledFace } };

						// DiceRolled 처리 중에 PlayerDiceAnimationStarted가 즉시 동기 발행될 수 있어
						// 대기 상태를 먼저 설정해 시작 이벤트를 놓치지 않도록 한다.
						m_WaitingForDoorRollConfirm = true;
						m_DoorRollAnimationObserved = false;
						m_DoorRollWaitTimer = 0.0f;

						std::cout << "[DoorFSM][Trace] Door_Attempt dispatch DiceRolled context=DoorRoll value=" << rolledFace << std::endl;
						GetEventDispatcher().Dispatch(EventType::DiceRolled, &individualEvent);

						const bool success = roll.total >= DoorRollThreshold;
						std::cout << "[DoorFSM][Trace] Door_Attempt set success=" << success
							<< " threshold=" << DoorRollThreshold << std::endl;
						player->SetDoorSuccess(success);
						hasRolled = true;
					}
				}
			}


			if (hasRolled)
			{
				m_RollPhaseLocked = true;
				m_PendingDoorVerdict = false;
				std::cout << "[DoorFSM][Trace] Door_Attempt waiting for dice animation before Door_Confirm"
					<< " observedStart=" << m_DoorRollAnimationObserved << std::endl;
			}
			else
			{
				std::cout << "[DoorFSM][Trace] Door_Attempt no roll available -> dispatch Door_Confirm immediately" << std::endl;
				DispatchEvent("Door_Confirm");
			}
		});

	BindActionHandler("Door_Select", [this](const FSMAction& action)
		{
			// 안내 UI
			std::cout << "Door Select\n";
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			//const bool confirmed = player ? player->ConsumeDoorConfirmed() : false;
			//DispatchEvent(confirmed ? "Door_Confirm" : "Door_Revoke");'
			GetEventDispatcher().Dispatch(EventType::PlayerDiceUIReset, nullptr);
			GetEventDispatcher().Dispatch(EventType::PlayerDoorInteract, nullptr);
		});
	BindActionHandler("Door_Verdict", [this](const FSMAction& action)
		{
			// 문 여는 거 판단
			if (m_WaitingForDoorRollConfirm)
			{
				m_PendingDoorVerdict = true;
				std::cout << "[DoorFSM][Trace] Door_Verdict deferred: waiting for Door_Confirm" << std::endl;
				return;
			}
			ResolveDoorVerdictNow();
		});

	BindActionHandler("Door_Open", [this](const FSMAction& action)
		{
			// 이동 가능하게 바꾸기
			// 애니메이션`1
			m_RollPhaseLocked = false;
			m_PendingDoorVerdict = false;
			m_WaitingForDoorRollConfirm = false;
			std::cout << "[DoorFSM][Trace] Door_Open" << std::endl;
			std::cout << "Door Success" << std::endl;
			if (auto* owner = GetOwner())
			{
				if (auto* player = owner->GetComponent<PlayerComponent>())
				{
					if (auto* door = player->ConsumePendingDoor())
					{
						door->OpenDoor();
					}
				}

// 				if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
// 				{
// 					playerFsm->DispatchEvent("Door_Complete");
// 				}

				auto* scene = owner->GetScene();
				if (scene)
				{
					auto& services = scene->GetServices();
					if (services.Has<SoundManager>())
					{
						services.Get<SoundManager>().SFX_Shot(L"Dice_Success");
					}
				}
			}
			std::cout << "[DoorFSM][Trace] Door_Open dispatch PlayerDoorSuccess" << std::endl;
			GetEventDispatcher().Dispatch(EventType::PlayerDoorSuccess, nullptr);
			//GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
			//DispatchEvent("Door_Complete");
			DispatchEvent("None");
		});
	BindActionHandler("Door_Fail", [this](const FSMAction& action)
		{
			m_RollPhaseLocked = false;
			m_PendingDoorVerdict = false;
			m_WaitingForDoorRollConfirm = false;
			std::cout << "[DoorFSM][Trace] Door_Fail" << std::endl;
			std::cout << "Door Fail" << std::endl;
			auto* owner = GetOwner();
			if (owner)
			{
				// 실패 시에는 PendingDoor를 유지해 팝업 종료 후 같은 문에 재도전할 수 있게 한다.
				auto* scene = owner->GetScene();
				if (scene)
				{
					auto& services = scene->GetServices();
					if (services.Has<SoundManager>())
					{
						services.Get<SoundManager>().SFX_Shot(L"Dice_Fail");
					}
				}
			}
			std::cout << "[DoorFSM][Trace] Door_Fail dispatch PlayerDoorFail" << std::endl;
			GetEventDispatcher().Dispatch(EventType::PlayerDoorFail, nullptr);

			// 실패 시 즉시 재도전으로 돌아가지 않고,
			// 팝업을 닫은 뒤 재진입 이벤트(Door_Select)로 다시 시작되게 한다.
			DispatchEvent("None");
		});

	BindActionHandler("Door_Revoke", [this](const FSMAction& action)
		{
			if (m_RollPhaseLocked)
			{
				std::cout << "[DoorFSM][Trace] Door_Revoke ignored: roll phase locked" << std::endl;
				return;
			}

			auto* owner = GetOwner();
			if (owner)
			{
				if (auto* player = owner->GetComponent<PlayerComponent>())
				{
					player->ConsumePendingDoor();
				}
				if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
				{
					playerFsm->DispatchEvent("Door_Complete");
				}

			}

			m_RollPhaseLocked = false;
			m_PendingDoorVerdict = false;
			m_WaitingForDoorRollConfirm = false;
			std::cout << "[DoorFSM][Trace] Door_Revoke dispatch PlayerDoorCancel" << std::endl;
			GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
			DispatchEvent("None");
		});
}


void PlayerDoorFSMComponent::Start()
{
	FSMComponent::Start();
	m_WaitingForDoorRollConfirm = false;
	m_DoorRollAnimationObserved = false;
	m_DoorRollWaitTimer = 0.0f;
	m_PendingDoorVerdict = false;
	m_RollPhaseLocked = false;

	if (!m_ListenersRegistered)
	{
		GetEventDispatcher().AddListener(EventType::PlayerDiceAnimationStarted, this);
		GetEventDispatcher().AddListener(EventType::PlayerDiceAnimationCompleted, this);
		GetEventDispatcher().AddListener(EventType::TurnChanged, this);
		m_ListenersRegistered = true;
	}
}

void PlayerDoorFSMComponent::Update(float deltaTime)
{
	FSMComponent::Update(deltaTime);

	if (!m_WaitingForDoorRollConfirm)
	{
		return;
	}

	m_DoorRollWaitTimer += deltaTime;
	if (!m_DoorRollAnimationObserved && m_DoorRollWaitTimer >= 0.8f)
	{
		std::cout << "[DoorFSM][Trace] Door_Attempt fallback confirm: no animation start observed in "
			<< m_DoorRollWaitTimer << "s" << std::endl;
		m_WaitingForDoorRollConfirm = false;
		DispatchEvent("Door_Confirm");
		if (m_PendingDoorVerdict)
		{
			std::cout << "[DoorFSM][Trace] replay deferred Door_Verdict after fallback confirm" << std::endl;
			ResolveDoorVerdictNow();
		}
		return;
	}

	if (m_DoorRollAnimationObserved && m_DoorRollWaitTimer >= 1.5f)
	{
		std::cout << "[DoorFSM][Trace] Door_Attempt fallback confirm: animation completion timeout "
			<< m_DoorRollWaitTimer << "s" << std::endl;
		m_WaitingForDoorRollConfirm = false;
		DispatchEvent("Door_Confirm");
		if (m_PendingDoorVerdict)
		{
			std::cout << "[DoorFSM][Trace] replay deferred Door_Verdict after completion-timeout confirm" << std::endl;
			ResolveDoorVerdictNow();
		}
	}
}


void PlayerDoorFSMComponent::ResolveDoorVerdictNow()
{
	m_PendingDoorVerdict = false;

	auto* owner = GetOwner();
	auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
	const bool success = player ? player->ConsumeDoorSuccess() : false;
	std::cout << "[DoorFSM][Trace] Door_Verdict success=" << success << std::endl;
	DispatchEvent(success ? "Door_Open" : "Door_Fail");
}

void PlayerDoorFSMComponent::OnEvent(EventType type, const void* data)
{
	FSMComponent::OnEvent(type, data);

	if (type == EventType::TurnChanged)
	{
		const auto* payload = static_cast<const Events::TurnChanged*>(data);
		if (payload && payload->turn != static_cast<int>(Turn::PlayerTurn))
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool isDoorInteractionActive = m_RollPhaseLocked
				|| m_WaitingForDoorRollConfirm
				|| GetCurrentStateName() != "None";
			if (isDoorInteractionActive)
			{
				m_RollPhaseLocked = false;
				m_PendingDoorVerdict = false;
				m_WaitingForDoorRollConfirm = false;
				m_DoorRollAnimationObserved = false;
				m_DoorRollWaitTimer = 0.0f;

				if (player)
				{
					player->ConsumePendingDoor();
				}

				std::cout << "[DoorFSM][Trace] Turn changed to enemy: force cancel door interaction" << std::endl;
				GetEventDispatcher().Dispatch(EventType::PlayerDoorCancel, nullptr);
				DispatchEvent("None");
			}
		}
		return;
	}

	if (!m_WaitingForDoorRollConfirm)
	{
		return;
	}

	if (type == EventType::PlayerDiceAnimationStarted)
	{
		m_DoorRollAnimationObserved = true;
		std::cout << "[DoorFSM][Trace] observed PlayerDiceAnimationStarted while waiting for Door_Confirm" << std::endl;
		return;
	}

	if (type == EventType::PlayerDiceAnimationCompleted && m_DoorRollAnimationObserved)
	{
		m_WaitingForDoorRollConfirm = false;
		std::cout << "[DoorFSM][Trace] observed PlayerDiceAnimationCompleted -> dispatch Door_Confirm" << std::endl;
		DispatchEvent("Door_Confirm");
		if (m_PendingDoorVerdict)
		{
			std::cout << "[DoorFSM][Trace] replay deferred Door_Verdict after animation completion" << std::endl;
			ResolveDoorVerdictNow();
		}
	}
}