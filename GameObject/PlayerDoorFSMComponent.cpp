#include "PlayerDoorFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerDoorFSMComponent, FSMComponent)

PlayerDoorFSMComponent::PlayerDoorFSMComponent()
{
	BindActionHandler("Door_ConsumeActResource", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				return;
			}

			const int amount = action.params.value("amount", 0);
			player->ConsumeActResource(amount);
		});
	BindActionHandler("Door_Attempt", [this](const FSMAction& action)
		{
			// 난이도 표시 UI
			// 주사위
		});
	BindActionHandler("Door_Select", [this](const FSMAction& action)
		{
			// 안내 UI
		});
	BindActionHandler("Door_Verdict", [this](const FSMAction& action)
		{
			// 문 여는 거 판단
		});
	BindActionHandler("Door_Open", [this](const FSMAction& action)
		{
			// 이동 가능하게 바꾸기
			// 애니메이션
		});
	BindActionHandler("Door_Fail", [this](const FSMAction& action)
		{
			// 아무것도 안함
		});
}


void PlayerDoorFSMComponent::Start()
{
	FSMComponent::Start();
}

void PlayerDoorFSMComponent::Update(float deltaTime)
{
	FSMComponent::Update(deltaTime);
}
