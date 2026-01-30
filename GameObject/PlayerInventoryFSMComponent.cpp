#include "PlayerInventoryFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerInventoryFSMComponent, FSMComponent)

PlayerInventoryFSMComponent::PlayerInventoryFSMComponent()
{
	BindActionHandler("Inventory_DropAttempt", [this](const FSMAction& action)
		{
			// 상점 여부 확인
		});
	BindActionHandler("Inventory_Drop", [this](const FSMAction& action)
		{
			// 버릴 수 있는지 확인
			// 버리기 가능한 곳 찾기 6개 모서리 랜덤 -> 시계방향 탐색
			// 빈 자리 메시 배치
		});
	BindActionHandler("Inventory_Sell", [this](const FSMAction& action)
		{
			// 환금율 계산
			// 재화 반영, 아이템 소멸
		});
}


void PlayerInventoryFSMComponent::Start()
{
	FSMComponent::Start();
}