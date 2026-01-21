#pragma once
#include "Component.h"

class SceneChangeTestComponent : public Component, public IEventListener {

	friend class Editor;

public :
	static constexpr const char* StaticTypeName = "SceneChangeTestComponent";
	const char* GetTypeName() const override;

	SceneChangeTestComponent();
	virtual ~SceneChangeTestComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SceneChange();
private:
	bool m_IsF12 = false;
};