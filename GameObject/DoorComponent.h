#pragma once
#include "Component.h"
#include "IEventListener.h"

class DoorComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "EnemyComponent";
	const char* GetTypeName() const override;

	DoorComponent();
	virtual ~DoorComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;
};

