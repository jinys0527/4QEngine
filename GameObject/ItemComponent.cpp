#include "ItemComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(ItemComponent)
REGISTER_PROPERTY(ItemComponent, ItemIndex)
REGISTER_PROPERTY(ItemComponent, IsEquiped)
REGISTER_PROPERTY(ItemComponent, Type)
REGISTER_PROPERTY(ItemComponent, Name)
REGISTER_PROPERTY(ItemComponent, IconPath)
REGISTER_PROPERTY(ItemComponent, MeshPath)
REGISTER_PROPERTY(ItemComponent, DescriptionIndex)
REGISTER_PROPERTY(ItemComponent, SellPrice)
REGISTER_PROPERTY(ItemComponent, MeleeAttackRange)
REGISTER_PROPERTY(ItemComponent, MaxDiceRoll)
REGISTER_PROPERTY(ItemComponent, MaxDiceValue)
REGISTER_PROPERTY(ItemComponent, BonusValue)
REGISTER_PROPERTY(ItemComponent, CON)
REGISTER_PROPERTY(ItemComponent, STR)
REGISTER_PROPERTY(ItemComponent, DEX)
REGISTER_PROPERTY(ItemComponent, SENSE)
REGISTER_PROPERTY(ItemComponent, TEC)
REGISTER_PROPERTY(ItemComponent, DEF)
REGISTER_PROPERTY(ItemComponent, ThrowRange)
REGISTER_PROPERTY(ItemComponent, DifficultyGroup)

ItemComponent::ItemComponent() = default;


ItemComponent::~ItemComponent()
{
}

void ItemComponent::Start()
{
}

void ItemComponent::Update(float deltaTime)
{
}

void ItemComponent::OnEvent(EventType type, const void* data)
{
}