#include "PlayerShopFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT_DERIVED(PlayerShopFSMComponent, FSMComponent)

PlayerShopFSMComponent::PlayerShopFSMComponent()
{
	BindActionHandler("Shop_ItemSelect", [this](const FSMAction&)
		{
			// SpaceCheck로 넘기기
		});
	BindActionHandler("Shop_SpaceCheck", [this](const FSMAction&)
		{
			// 빈칸 여부 확인
		});
	BindActionHandler("Shop_MoneyCheck", [this](const FSMAction&)
		{
			// 재화 확인
		});
	BindActionHandler("Shop_Buy", [this](const FSMAction&)
		{
			// 할인율 계산
			// 재화 반영
			// 인벤토리 갱신
		});
}

void PlayerShopFSMComponent::Start()
{
	FSMComponent::Start();
}