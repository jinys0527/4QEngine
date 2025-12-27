#include "RigidbodyComponent.h"

void RigidbodyComponent::Update(float deltaTime)
{
}

void RigidbodyComponent::OnEvent(EventType type, const void* data)
{
}

void RigidbodyComponent::Serialize(nlohmann::json& j) const
{
}

void RigidbodyComponent::Deserialize(const nlohmann::json& j)
{
}

void RigidbodyComponent::Integrate(float deltaTime)
{
	if (m_IsKinematic || !m_Transform)
		return;

	if (m_Force.y >= 3.0f)
		m_Force = m_Force;

	//중력 적용
	Vec2F totalForce = m_Force + m_Gravity * m_Mass;

	//마찰력 적용
	if (m_Velocity.LengthSquared() > 0.00001f)
	{
		Vec2F frictionDir = -m_Velocity.Normalized();
		Vec2F friction = frictionDir * m_FrictionCoefficient * m_Mass * 9.8f; // N ? mass * g
		totalForce += friction;
	}

	Vec2F acceleration = (m_Mass > 0.0f) ? (totalForce / m_Mass) : Vec2F(0, 0);
	m_Velocity += acceleration * deltaTime;
	Vec2F newPos = m_Transform->GetPosition() + m_Velocity;// 여기서 한번더 deltatime 곱하던데 그러면 delta 두번 곱한거라 엄청 느려졌음
	m_Transform->SetPosition(newPos);

	// 너무 느리면 정지 (정밀도 오차 방지)
	if (m_Velocity.LengthSquared() < 0.00001f)
		m_Velocity = Vec2F(0, 0);

	m_Force = Vec2F(0, 0);
}
