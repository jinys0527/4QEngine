#pragma once
#include "Component.h"
#include "GameState.h"

class GridSystemComponent;
class EnemyComponent;

class EnemyControllerComponent : public Component, public IEventListener {

	friend class Editor;
public: 
	static constexpr const char* StaticTypeName = "EnemyControllerComponent";
	const char* GetTypeName() const override;

	EnemyControllerComponent() = default;
	virtual ~EnemyControllerComponent();

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

private:

	void GetSystem();
	bool CheckActiveEnemies();

	GridSystemComponent* m_GridSystem = nullptr;
	bool m_TurnEndRequested = false;
};
