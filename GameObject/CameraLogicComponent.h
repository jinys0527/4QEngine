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

	CameraLogicComponent() = default;
	virtual ~CameraLogicComponent();

	void SetMaxZoom(const float& value) { m_MaxZoom = value; }
	void SetMinZoom(const float& value) { m_MinZoom = value; }
	void SetXOffset(const float& value) { m_XOffset = value; }
	void SetThreshold(const float& value) { m_FollowThreshold = value; }

	const float& GetMaxZoom() const { return m_MaxZoom; }
	const float& GetMinZoom() const { return m_MinZoom; }
	const float& GetXOffset() const { return m_XOffset; }
	const float& GetThreshold() const { return m_FollowThreshold; }


	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;
	void CamZoom(); 
	void CamFollowX(float deltaTime);

private:

	float m_MaxZoom= 0.0f;
	float m_MinZoom = 0.0f;
	float m_XOffset = 2.0f;
	float m_ZoomSpeed = 1.0f;
	float m_FollowThreshold = 3.0f;

	TransformComponent* m_Transform = nullptr;
	CameraComponent*    m_Camera = nullptr;
	TransformComponent* m_PlayerTransform = nullptr;
	float m_PendingZoomInput = 0.0f;
};