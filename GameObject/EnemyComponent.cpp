#include "EnemyComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT(EnemyComponent)
REGISTER_PROPERTY_READONLY(EnemyComponent, Q)
REGISTER_PROPERTY_READONLY(EnemyComponent, R)

EnemyComponent::EnemyComponent() {

}

EnemyComponent::~EnemyComponent() {
	// Event Listener 쓰는 경우만

}

void EnemyComponent::Start()
{
}

void EnemyComponent::Update(float deltaTime) {


}

void EnemyComponent::OnEvent(EventType type, const void* data)
{

}
