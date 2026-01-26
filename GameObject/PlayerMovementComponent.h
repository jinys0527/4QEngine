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

	float m_Speed      = 5.0f;

	bool  m_IsDragging = false;

	float m_DragSpeed  = 0.01f;

	bool m_HasDragRay = false;
	DirectX::XMFLOAT3 m_DragRayOrigin{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_DragRayDir   { 0.0f, 0.0f, 0.0f };

	// 바닥 평면 기반 오프셋/고정 Y
	DirectX::XMFLOAT3 m_DragOffset	 { 0.0f, 0.0f, 0.0f };
};

