#pragma once
#include "Component.h"
#include <DirectXMath.h>

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
	const float& GetSpeed() const     { return m_Speed;  }

	void SetDragSpeed(const float& dragSpeed) { m_DragSpeed = dragSpeed; }
	const float& GetDragSpeed() const { return m_DragSpeed; }

private:
	bool  m_IsUp       = false;
	bool  m_IsLeft     = false;
	bool  m_IsDown     = false;
	bool  m_IsRight    = false;
	bool  m_IsDragging = false;
	POINT m_LastMousePos{ 0, 0 };
	float m_Speed      = 5.0f;
	float m_DragSpeed  = 0.01f;
	DirectX::XMFLOAT3 m_DragOffset{ 0.0f, 0.0f, 0.0f };
	float m_DragPlaneY = 0.0f;
};

