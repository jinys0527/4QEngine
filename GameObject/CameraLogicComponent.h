#pragma once

#include "Component.h"
#include "GameObject.h"

class CameraComponent;
class TransformComponent;

// Game에서의 카메라 움직임, 시야변경을 다룸
class CameraLogicComponent : public Component, public IEventListener {
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "CameraLogicComponent";
	const char* GetTypeName() const override;

	CameraLogicComponent() =default;
	virtual ~CameraLogicComponent() = default;

	void SetMaxZoom(const float& value) { m_MaxZoom = value; }
	void SetMinZoom(const float& value) { m_MinZoom = value; }
	void SetMoveSpeed(const float& value) { m_MoveSpeed = value; }

	const float& GetMaxZoom() const { return m_MaxZoom; }
	const float& GetMinZoom() const { return m_MinZoom; }
	const float& GetMoveSpeed() const { return m_MoveSpeed; }

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

private:

	float m_MaxZoom= 0.0f;
	float m_MinZoom = 0.0f;
	float m_MoveSpeed = 2.0f;
	float m_ZoomSpeed = 2.0f;

};