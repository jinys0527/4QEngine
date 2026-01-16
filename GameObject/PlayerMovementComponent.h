#pragma once
#include "Component.h"

class PlayerMovementComponent : public Component, public IEventListener
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "PlayerMovementComponent";
	const char* GetTypeName() const override;

	PlayerMovementComponent();
	virtual ~PlayerMovementComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetSpeed(const float& speed) { m_Speed = speed; }
	const float& GetSpeed() const { return m_Speed; }

private:
	bool m_IsUp    = false;
	bool m_IsLeft  = false;
	bool m_IsDown  = false;
	bool m_IsRight = false;
	float m_Speed = 5.0f;
};

