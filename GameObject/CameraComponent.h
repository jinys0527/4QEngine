#pragma once
#include "Component.h"
#include "MathHelper.h"
using namespace MathUtils;

enum class ProjectionMode
{
	Perspective,
	Orthographic,
	OrthoOffCenter
};

class CameraComponent : public Component
{
	friend class Editor;

public:
	struct Viewport
	{
		float Width = 0.0f;
		float Height = 0.0f;
	};

	struct PerspectiveParams
	{
		float Fov = XM_PIDIV4;
		float Aspect = 1.0f;
	};

	struct OrthoParams
	{
		float Width = 10.0f;
		float Height = 10.0f;
	};

	struct OrthoOffCenterParams
	{
		float Left = -1.0f;
		float Right = 1.0f;
		float Bottom = -1.0f; ;
		float Top = 1.0f;
	};

protected:
	// 내부 재계산
	void RebuildViewIfDirty    ();
	void RebuildProjIfDirty    ();

protected:
	// Viewport(또는 화면) 관련
	Viewport m_ViewportSize;


	float m_NearZ  = 0.1f;
	float m_FarZ   = 1000.0f;

	PerspectiveParams m_Persp;

	OrthoParams m_Ortho;

	OrthoOffCenterParams m_OrthoOC;


	XMFLOAT4X4 m_View = Identity();
	XMFLOAT4X4 m_Proj = Identity();

	ProjectionMode m_Mode = ProjectionMode::Perspective;

	bool m_ViewDirty = true;
	bool m_ProjDirty = true;

	XMFLOAT3 m_Eye  { 0.0f, 0.0f, -5.0f };
	//Look : 방향벡터 X, Target Point
	XMFLOAT3 m_Look { 0.0f, 0.0f,  0.0f };
	XMFLOAT3 m_Up   { 0.0f, 1.0f,  0.0f };


public:
	CameraComponent() = default;
	CameraComponent(float viewportW, float viewportH,
		float fov, float aspect, float nearZ, float farZ, ProjectionMode mode);

	CameraComponent(float viewportW, float viewportH,
		float left, float right, float bottom, float top, float nearZ, float farZ);
	virtual ~CameraComponent() = default;

	static constexpr const char* StaticTypeName = "CameraComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize  (nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	Viewport GetViewportSize() const
	{
		return m_ViewportSize;
	}


	// Get할때만 계산해서 받아옴
	XMFLOAT4X4 GetViewMatrix  ();
	XMFLOAT4X4 GetProjMatrix  ();

	void SetEyeLookUp(const XMFLOAT3& eye, const XMFLOAT3& look, const XMFLOAT3& up);
	const XMFLOAT3& GetEye () const { return m_Eye;  }
	const XMFLOAT3& GetLook() const { return m_Look; }
	const XMFLOAT3& GetUp  () const { return m_Up;   }

	void SetPerspectiveProj   (const float fov, const float aspect, const float nearZ, const float farZ);
	void SetOrthoProj         (const float width, const float height, const float nearZ, const float farZ);
	void SetOrthoOffCenterProj(const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ);

	// 화면 크기 들어오면 aspect 갱신하고(옵션) ortho width/height도 동기화 가능
	void SetViewportSize      (float width, float height);

};

REGISTER_COMPONENT(CameraComponent);