#pragma once
#include "Component.h"
#include "GameState.h"

#include <memory>

class AIController;
class BTExecutor;
class TransformComponent;
class MaterialComponent;

class EnemyComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "EnemyComponent";
	const char* GetTypeName() const override;

	EnemyComponent();
	virtual ~EnemyComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요

	void SetQR(int q, int r) { m_Q = q, m_R = r; }
	const int& GetQ() const { return m_Q; }
	const int& GetR() const { return m_R; }

	void SetMoveDistance(const int& value) { m_MoveDistance = value; }
	const int& GetMoveDistance() const { return m_MoveDistance; }
	Turn GetCurrentTurn() const { return m_CurrentTurn; }
	bool ConsumeMoveRequest();

private:
	int m_Q;
	int m_R;
	int m_MoveDistance = 1;
	Turn m_CurrentTurn = Turn::PlayerTurn;

	std::unique_ptr<BTExecutor>   m_BTExecutor;
	std::unique_ptr<AIController> m_AIController;
	TransformComponent* m_TargetTransform = nullptr;
	bool m_MoveRequested = false;
};