#pragma once
#include "Component.h"

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

private:
	int m_Q;
	int m_R;
	int m_MoveDistance = 1;

};