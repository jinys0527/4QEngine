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

	// FSM/다른 컴포넌트가 조회하는 입력 상태(프리뷰/판정은 FSM에서 처리)
	bool  HasDragRay() const { return m_HasDragRay; }
	const DirectX::XMFLOAT3& GetDragRayOrigin() const { return m_DragRayOrigin; }
	const DirectX::XMFLOAT3& GetDragRayDir() const { return m_DragRayDir; }
	const DirectX::XMFLOAT3& GetDragOffset() const { return m_DragOffset; }
	const DirectX::XMFLOAT3& GetDragStartPos() const { return m_DragStartPos; }
	NodeComponent* GetDragStartNode() const { return m_DragStartNode; }

	bool IsDragging() const;

private:
	// ProPerty
	float m_DragSpeed = 0.01f;

	// 입력 레이만 유지 (드래그 활성 여부는 FSM이 관리)
	bool m_HasDragRay = false;
	DirectX::XMFLOAT3 m_DragRayOrigin{ 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_DragRayDir   { 0.0f, 0.0f, 0.0f };

	// 바닥 평면 기반 오프셋/고정 Y
	DirectX::XMFLOAT3 m_DragOffset	 { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_DragStartPos{ 0.0f, 0.0f, 0.0f };
	NodeComponent* m_DragStartNode = nullptr;

	GridSystemComponent* m_GridSystem = nullptr;
};

