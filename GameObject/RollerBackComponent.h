#pragma once

#include "Component.h"
#include <functional>

#include "RectTransformComponent.h"

class Scene;

class RollerBackComponent : public Component, public IEventListener
{
public:
	RollerBackComponent() = default;
	~RollerBackComponent() = default;

	static constexpr const char* StaticTypeName = "RollerBackComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	void Start();

	void SetScenePtr(Scene* ptr) { scenePtr = ptr; }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void SetOpacity(float value) { m_Opacity = value; }

private:
	float m_Opacity = 1;
	bool m_IsStart = false;

	Scene* scenePtr;

	RectTransformComponent* m_RectTransComp;

};

