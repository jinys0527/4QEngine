#include "PlayerStatComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(PlayerStatComponent, StatComponent)
REGISTER_PROPERTY(PlayerStatComponent, Health)
REGISTER_PROPERTY(PlayerStatComponent, Strength)
REGISTER_PROPERTY(PlayerStatComponent, Agility)
REGISTER_PROPERTY(PlayerStatComponent, Sense)
REGISTER_PROPERTY(PlayerStatComponent, Skill)
REGISTER_PROPERTY_READONLY(PlayerStatComponent, EquipmentDefenseBonus)


void PlayerStatComponent::Update(float deltaTime)
{
}

void PlayerStatComponent::OnEvent(EventType type, const void* data)
{
}

const int PlayerStatComponent::CalculateStatModifier(int statValue) const
{
	int diff = statValue - 12;
	if (diff >= 0)
	{
		return diff / 2;
	}
	return -diff / 2;
}

float PlayerStatComponent::GetShopDiscountRate() const
{
	const int modifier = GetCalculatedSkillModifier();
	return static_cast<const float>(modifier) / 10.0f;
}

const int PlayerStatComponent::GetMaxHealthForFloor(int currentFloor) const
{
	return m_Health + (GetCalculatedHealthModifier() * currentFloor);
}
