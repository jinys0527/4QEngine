#pragma once
#include "Component.h"
#include <DirectXMath.h>

class GridSystemComponent;
class EnemyComponent;
class TransformComponent;

enum class ERotationOffset {
	clock_1,
	clock_3,
	clock_5,
	clock_7,
	clock_9,
	clock_11,
};

// 이벤트 리스너는 쓸 얘들만
class EnemyMovementComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "EnemyMovementComponent";
	const char* GetTypeName() const override;

	EnemyMovementComponent()=default;
	virtual ~EnemyMovementComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요
	

	void Move();
	bool IsMoveComplete() const { return m_IsMoveComplete;  }

private:

	void SetEnemyRotation(TransformComponent* transComp, ERotationOffset dir);

	void GetSystem();
	bool m_IsMoveComplete = false;
	GridSystemComponent* m_GridSystem = nullptr;
};