#pragma once
#include "Component.h"


class ExitComponent : public Component {
	friend class Editor;

public :
	static constexpr const char* StaticTypeName = "ExitComponent";
	const char* GetTypeName() const override;
	ExitComponent() = default;
	virtual ~ExitComponent() = default;


	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;
};