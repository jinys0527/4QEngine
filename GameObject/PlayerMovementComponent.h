#pragma once
#include "Component.h"
#include <DirectXMath.h>
class NodeComponent;
class GridSystemComponent;

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

	void SetDragSpeed(const float& dragSpeed) { m_DragSpeed = dragSpeed; }
	const float& GetDragSpeed() const { return m_DragSpeed; }
	bool IsDragging() const { return m_IsDragging; }

	NodeComponent* GetDragStartNode() const { return m_DragStartNode; }

private:
	// ProPerty
	float m_DragSpeed = 0.01f;

	bool  m_IsDragging = false;
	bool m_HasDragRay = false;
	DirectX::XMFLOAT3 m_DragRayOrigin{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_DragRayDir   { 0.0f, 0.0f, 0.0f };
	// 바닥 평면 기반 오프셋/고정 Y
	DirectX::XMFLOAT3 m_DragOffset	 { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_DragStartPos{ 0.0f, 0.0f, 0.0f };
	NodeComponent* m_PendingNode = nullptr;
	NodeComponent* m_CurrentTargetNode = nullptr;
	NodeComponent* m_DragStartNode = nullptr;
	GridSystemComponent* m_GridSystem = nullptr;
};

