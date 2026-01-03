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
	j["mode"] = static_cast<int>(m_Mode);
	j["viewport"]["width"]  = m_ViewportSize.Width;
	j["viewport"]["height"] = m_ViewportSize.Height;
	j["nearZ"] = m_NearZ;
	j["farZ"]  = m_FarZ;

	j["eye"]["x"]  = m_Eye.x;
	j["eye"]["y"]  = m_Eye.y;
	j["eye"]["z"]  = m_Eye.z;

	j["look"]["x"] = m_Look.x;
	j["look"]["y"] = m_Look.y;
	j["look"]["z"] = m_Look.z;

	j["up"]["x"]   = m_Up.x;
	j["up"]["y"]   = m_Up.y;
	j["up"]["z"]   = m_Up.z;

	switch (m_Mode)
	{
	case ProjectionMode::Perspective:
		j["perspective"]["fov"]       = m_Persp.Fov;
		j["perspective"]["aspect"]    = m_Persp.Aspect;
		break;
	case ProjectionMode::Orthographic:
		j["orthographic"]["width"]    = m_Ortho.Width;
		j["orthographic"]["height"]   = m_Ortho.Height;
		break;
	case ProjectionMode::OrthoOffCenter:
		j["orthoOffCenter"]["left"]   = m_OrthoOC.Left;
		j["orthoOffCenter"]["right"]  = m_OrthoOC.Right;
		j["orthoOffCenter"]["bottom"] = m_OrthoOC.Bottom;
		j["orthoOffCenter"]["top"]    = m_OrthoOC.Top;
		break;
	}
}

void CameraComponent::Deserialize(const nlohmann::json& j)
{
	m_Mode = static_cast<ProjectionMode>(j.value("mode", static_cast<int>(ProjectionMode::Perspective)));
	if (j.contains("viewport"))
	{
		m_ViewportSize.Width  = j["viewport"].value("width", m_ViewportSize.Width);
		m_ViewportSize.Height = j["viewport"].value("height", m_ViewportSize.Height);
	}

	m_NearZ = j.value("nearZ", m_NearZ);
	m_FarZ  = j.value("farZ", m_FarZ);

	if (j.contains("eye"))
	{
		m_Eye.x = j["eye"].value("x", m_Eye.x);
		m_Eye.y = j["eye"].value("y", m_Eye.y);
		m_Eye.z = j["eye"].value("z", m_Eye.z);
	}

	if (j.contains("look"))
	{
		m_Look.x = j["look"].value("x", m_Look.x);
		m_Look.y = j["look"].value("y", m_Look.y);
		m_Look.z = j["look"].value("z", m_Look.z);
	}

	if (j.contains("up"))
	{
		m_Up.x = j["up"].value("x", m_Up.x);
		m_Up.y = j["up"].value("y", m_Up.y);
		m_Up.z = j["up"].value("z", m_Up.z);
	}

	switch (m_Mode)
	{
	case ProjectionMode::Perspective:
		if(j.contains("perspective"))
		{
			m_Persp.Fov    = j["perspective"].value("fov", m_Persp.Fov);
			m_Persp.Aspect = j["perspective"].value("aspect", m_Persp.Aspect);
		}
		break;
	case ProjectionMode::Orthographic:
		if(j.contains("orthographic"))
		{
			m_Ortho.Width = j["orthographic"].value("width", m_Ortho.Width);
			m_Ortho.Height = j["orthographic"].value("height", m_Ortho.Height);
		}
		break;
	case ProjectionMode::OrthoOffCenter:
		if (j.contains("orthoOffCenter"))
		{
			m_OrthoOC.Left   = j["orthoOffCenter"].value("left", m_OrthoOC.Left);
			m_OrthoOC.Right  = j["orthoOffCenter"].value("right", m_OrthoOC.Right);
			m_OrthoOC.Bottom = j["orthoOffCenter"].value("bottom", m_OrthoOC.Bottom);
			m_OrthoOC.Top    = j["orthoOffCenter"].value("top", m_OrthoOC.Top);
		}
		break;
	}

	m_ViewDirty = true;
	m_ProjDirty = true;
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
