#include "CircleColliderComponent.h"
#include "BoxColliderComponent.h"

bool CircleColliderComponent::CircleVsCircle(const CircleColliderComponent& other) const
{
	float distSq = (this->m_Center - other.m_Center).LengthSquared();
	float radiusSum = this->m_Radius + other.m_Radius;

	return distSq <= radiusSum * radiusSum;
}

bool CircleColliderComponent::CircleVsBox(const BoxColliderComponent& other) const
{
	Vec2F boxMin = other.m_Center - other.m_Size / 2;
	Vec2F boxMax = other.m_Center + other.m_Size / 2;
	Vec2F closest = Clamp(this->m_Center, boxMin,  boxMax);

	float distSq = (this->m_Center - closest).LengthSquared();

	return distSq <= this->m_Radius * this->m_Radius;
}

void CircleColliderComponent::Update(float deltaTime)
{
}

void CircleColliderComponent::OnEvent(EventType type, const void* data)
{
}

void CircleColliderComponent::Serialize(nlohmann::json& j) const
{
}

void CircleColliderComponent::Deserialize(const nlohmann::json& j)
{
}
