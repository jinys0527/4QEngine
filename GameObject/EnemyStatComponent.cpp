#include "EnemyStatComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(EnemyStatComponent, StatComponent)
REGISTER_PROPERTY(EnemyStatComponent, Defense)
REGISTER_PROPERTY(EnemyStatComponent, InitiativeModifier)
REGISTER_PROPERTY(EnemyStatComponent, AccuracyModifier)
REGISTER_PROPERTY(EnemyStatComponent, DiceRollCount)
REGISTER_PROPERTY(EnemyStatComponent, MaxDiceValue)
REGISTER_PROPERTY(EnemyStatComponent, SightDistance)
REGISTER_PROPERTY(EnemyStatComponent, SightAngle)
REGISTER_PROPERTY(EnemyStatComponent, DifficultyGroup)

void EnemyStatComponent::Update(float deltaTime)
{
}

void EnemyStatComponent::OnEvent(EventType type, const void* data)
{
}
