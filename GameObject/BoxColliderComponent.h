#pragma once
#include "ColliderComponent.h"
#include "SimpleMathHelper.h"

class BoxColliderComponent : public ColliderComponent
{
	using Vec2F = Math::Vector2F;
	friend class CircleColliderComponent;
public:
	static constexpr const char* StaticTypeName = "BoxColliderComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	virtual ~BoxColliderComponent() { OnDestroy(); }
	void Start() override;
	bool BoxVsBox(const BoxColliderComponent& other) const;
	void SetSize(const Vec2F& size) {
		m_Size = size;
	}
	Vec2F GetCenter();

	void OnTrigger()
	{
		m_OnTrigger = nullptr;
	}

	const Vec2F& GetSize() const {
		return m_Size;
	}
	
	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;
	void OnDestroy() override;
protected:
	Vec2F m_Size;
	Vec2F m_SizeRatio;
};

REGISTER_COMPONENT(BoxColliderComponent);