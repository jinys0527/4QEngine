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

private:


};