#include "ReflectionMacro.h"

#include "LootRoller.h"

#include "ItemSpawnerComponent.h"

REGISTER_COMPONENT(ItemSpawnerComponent)


void ItemSpawnerComponent::Start()
{
    SpwanFixedItem();
}

void ItemSpawnerComponent::Update(float deltaTime)
{
}

void ItemSpawnerComponent::OnEvent(EventType type, const void* data)
{
}

void ItemSpawnerComponent::SpwanFixedItem()
{
}

void ItemSpawnerComponent::DropItem()
{
    //LootRoller lootRoller;

    //std::optional<LootRollResult> result =
    //    lootRoller.RollDrop(enemy, *m_GameDataRepository, diceSystem);

    //if (!result)
    //{
    //    return; // 드랍 없음
    //}

    //int itemIndex = result->itemIndex;
    //// 여기서 itemIndex로 스폰 처리
}