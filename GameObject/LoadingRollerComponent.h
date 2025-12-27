#pragma once

#include "Component.h"
#include <functional>

#include "RectTransformComponent.h"

class Scene;

class LoadingRollerComponent : public Component, public IEventListener
{
public:
	LoadingRollerComponent() = default;
	~LoadingRollerComponent() = default;

	static constexpr const char* StaticTypeName = "LoadingRollerComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	void Start();

	void SetIndex(int value) { m_Index = value; }
	void SetScenePtr(Scene* ptr) { scenePtr = ptr; }

	void SetFunc(std::function<void()> func) { m_SceneLoadFunc = func; }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetOpacity(float value) { m_Opacity = value; }

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

private:
	float m_Opacity = 1;
	bool m_IsLoaded = false;
	bool m_IsStart = false;
	int m_Index = 0;

	Scene* scenePtr;

	std::function<void()> m_SceneLoadFunc;
	RectTransformComponent* m_RectTransComp;

};

