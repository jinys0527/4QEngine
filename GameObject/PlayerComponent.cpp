#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"

REGISTER_COMPONENT(PlayerComponent)

PlayerComponent::PlayerComponent() {

}

PlayerComponent::~PlayerComponent() {
	// Event Listener 쓰는 경우만

}

void PlayerComponent::Start()
{
}

void PlayerComponent::Update(float deltaTime) {


}

void PlayerComponent::OnEvent(EventType type, const void* data)
{

}
