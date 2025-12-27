#include "CameraComponent.h"

void CameraComponent::RebuildViewIfDirty()
{
	if (!m_ViewDirty) return;

	XMStoreFloat4x4(&m_View, LookAtLH(m_Eye, m_Look, m_Up));

	m_ViewDirty = false;
}

void CameraComponent::RebuildProjIfDirty()
{
	if (!m_ProjDirty) return;

	switch (m_Mode)
	{
	case ProjectionMode::Perspective:
		XMStoreFloat4x4(&m_Proj, PerspectiveLH(m_Persp.Fov, m_Persp.Aspect, m_NearZ, m_FarZ));
		break;
	case ProjectionMode::Orthographic:
		XMStoreFloat4x4(&m_Proj, OrthographicLH(m_Ortho.Width, m_Ortho.Height, m_NearZ, m_FarZ));
		break;
	case ProjectionMode::OrthoOffCenter:
		XMStoreFloat4x4(&m_Proj, OrthographicOffCenterLH(m_OrthoOC.Left, m_OrthoOC.Right, m_OrthoOC.Bottom, m_OrthoOC.Top, m_NearZ, m_FarZ));
		break;
	}
	
	m_ProjDirty = false;
}

CameraComponent::CameraComponent(float viewportW, float viewportH,
	float param1, float param2, float nearZ, float farZ, ProjectionMode mode) : m_ViewportSize({ viewportW, viewportH }), m_NearZ(nearZ), m_FarZ(farZ), m_Mode(mode)
{
	switch (mode)
	{
	case ProjectionMode::Perspective:
		SetPerspectiveProj(param1, param2, nearZ, farZ);
		break;
	case ProjectionMode::Orthographic:
		SetOrthoProj(param1, param2, nearZ, farZ);
		break;
	}
}


CameraComponent::CameraComponent(float viewportW, float viewportH,
	float left, float right, float bottom, float top, float nearZ, float farZ) : m_ViewportSize({ viewportW, viewportH }), m_OrthoOC({ left, right, bottom, top }), m_NearZ(nearZ), m_FarZ(farZ)
{
	SetOrthoOffCenterProj(left, right, bottom, top, nearZ, farZ);
}

void CameraComponent::Update(float deltaTime)
{
}

void CameraComponent::OnEvent(EventType type, const void* data)
{
}

void CameraComponent::Serialize(nlohmann::json& j) const
{
}

void CameraComponent::Deserialize(const nlohmann::json& j)
{
}

XMFLOAT4X4 CameraComponent::GetViewMatrix()
{
	RebuildViewIfDirty(); // 필요할 때만 계산
	return m_View;
}

XMFLOAT4X4 CameraComponent::GetProjMatrix()
{
	RebuildProjIfDirty(); // 필요할 때만 계산
	return m_Proj;
}

void CameraComponent::SetEyeLookUp(const XMFLOAT3& eye, const XMFLOAT3& look, const XMFLOAT3& up)
{
	m_Eye  = eye;
	m_Look = look;
	m_Up   = up;
	m_ViewDirty = true;
}



void CameraComponent::SetPerspectiveProj(const float fov, const float aspect, const float nearZ, const float farZ)
{
	m_Mode = ProjectionMode::Perspective;
	m_Persp.Fov = fov;
	m_Persp.Aspect = aspect;
	m_NearZ = nearZ;
	m_FarZ = farZ;
	m_ProjDirty = true;
}

void CameraComponent::SetOrthoProj(const float width, const float height, const float nearZ, const float farZ)
{
	m_Mode = ProjectionMode::Orthographic;
	m_Ortho.Width = width;
	m_Ortho.Height = height;
	m_NearZ = nearZ;
	m_FarZ = farZ;
	m_ProjDirty = true;
}

void CameraComponent::SetOrthoOffCenterProj(const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
	m_Mode = ProjectionMode::OrthoOffCenter;
	m_OrthoOC.Left = left;
	m_OrthoOC.Right = right;
	m_OrthoOC.Bottom = bottom;
	m_OrthoOC.Top = top;
	m_NearZ = nearZ;
	m_FarZ = farZ;
	m_ProjDirty = true;
}

void CameraComponent::SetViewportSize(float width, float height)
{
	m_ViewportSize = { width, height };

	if (m_Mode == ProjectionMode::Perspective && height > 0.0f)
	{
		m_Persp.Aspect = width / height;
		m_ProjDirty = true;
	}
}
