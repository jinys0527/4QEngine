#pragma once
#include "Component.h"

class PlayerComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "PlayerComponent";
	const char* GetTypeName() const override;

	PlayerComponent();
	virtual ~PlayerComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요

private:


};