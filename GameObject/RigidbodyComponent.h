#pragma once
#include "Component.h"
#include "SimpleMathHelper.h"
#include "GameObject.h"
#include "TransformComponent.h"

class RigidbodyComponent : public Component
{
	using Vec2F = Math::Vector2F;
public:
	static constexpr const char* StaticTypeName = "RigidbodyComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	RigidbodyComponent(float mass = 1.0f, bool isKinematic = false) : m_Mass(mass), m_IsKinematic(isKinematic), m_Velocity(0, 0), m_Force(0, 0), m_Gravity(0.0f, -9.8f), m_FrictionCoefficient(0.1f)
	{
	}
	
	void Start()
	{
		m_Transform = m_Owner->GetComponent<TransformComponent>();
	}

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void AddForce(const Vec2F& f)
	{
		if (!m_IsKinematic)
		{
			m_Force += f;
		}
	}

	void Integrate(float deltaTime);

	void SetVelocity(const Vec2F& vel)
	{
		m_Velocity = vel;
	}

	const Vec2F& GetVelocity() const 
	{
		return m_Velocity;
	}
	
	void SetIsKinematic(bool value)
	{
		m_IsKinematic = value;
	}

	bool IsKinematic() const { return m_IsKinematic; }

	void SetGravity(const Vec2F& g) { m_Gravity = g; }
	void SetFriction(float friction) { m_FrictionCoefficient = friction; }
private:
	float m_Mass;
	bool m_IsKinematic;

	Vec2F m_Velocity;
	Vec2F m_Force;
	Vec2F m_Gravity;
	float m_FrictionCoefficient;

	TransformComponent* m_Transform = nullptr;
};

REGISTER_COMPONENT(RigidbodyComponent);