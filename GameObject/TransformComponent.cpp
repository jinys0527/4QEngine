#include "TransformComponent.h"
#include "Event.h"
#include <cassert>
#include "RectTransformComponent.h"

void TransformComponent::SetParent(TransformComponent* newParent)
{
	assert(newParent != this);
	assert(m_Parent == nullptr);

	m_Parent = newParent;
	m_Parent->AddChild(this);

	SetDirty();
}

void TransformComponent::DetachFromParent()
{
	if (m_Parent == nullptr) return;

	m_Parent->RemoveChild(this);

	m_Parent = nullptr;

	SetDirty();
}

void TransformComponent::AddChild(TransformComponent* child)
{
	Matrix3X2F childLocalTM = child->GetLocalMatrix();
	childLocalTM = childLocalTM * GetInverseWorldMatrix();

	auto m_noPivot = TM::RemovePivot(childLocalTM, child->GetPivotPoint());
	TM::DecomposeMatrix3X2(m_noPivot, child->m_Position, child->m_Rotation, child->m_Scale);

	m_Children.push_back(child);
}

void TransformComponent::RemoveChild(TransformComponent* child)
{
	Matrix3X2F childLocalTM = child->GetLocalMatrix();
	childLocalTM = childLocalTM * GetWorldMatrix();

	auto m_noPivot = TM::RemovePivot(childLocalTM, child->GetPivotPoint());
	TM::DecomposeMatrix3X2(m_noPivot, child->m_Position, child->m_Rotation, child->m_Scale);

	m_Children.erase(
		std::remove(m_Children.begin(), m_Children.end(), child),
		m_Children.end()
	);
}

void TransformComponent::Translate(const Vec2F& delta)
{
	m_Position.x += delta.x;
	m_Position.y += delta.y;
	SetDirty();
}

void TransformComponent::Translate(const float x, const float y)
{
	m_Position.x += x;
	m_Position.y += y;
	SetDirty();
}

void TransformComponent::Rotate(float deg)
{
	m_Rotation += deg;
	SetDirty();
}

TransformComponent::Vec2F TransformComponent::GetForward() const
{
	float radian = Math::DegToRad(m_Rotation);
	return { std::cosf(radian), std::sinf(radian) };
}

const TransformComponent::Matrix3X2F& TransformComponent::GetWorldMatrix()
{
	if (m_IsDirty) UpdateMatrices();
	return m_WorldMatrix;
}

const TransformComponent::Matrix3X2F& TransformComponent::GetLocalMatrix()
{
	if (m_IsDirty) UpdateMatrices();
	return m_LocalMatrix;
}

TransformComponent::Matrix3X2F TransformComponent::GetInverseWorldMatrix()
{
	Matrix3X2F inv = GetWorldMatrix();
	inv.Invert();
	return inv;
}

void TransformComponent::SetPivotPreset(TransformPivotPreset preset, const D2D1_SIZE_F& size)
{
	switch (preset)
	{
	case TransformPivotPreset::TopLeft:
		m_Pivot = { 0.0f, 0.0f };
		break;
	case TransformPivotPreset::TopRight:
		m_Pivot = { size.width, 0.0f };
		break;
	case TransformPivotPreset::BottomLeft:
		m_Pivot = { 0.0f, size.height };
		break;
	case TransformPivotPreset::BottomRight:
		m_Pivot = { size.width, size.height };
		break;
	case TransformPivotPreset::Center:
		m_Pivot = { size.width * 0.5f, size.height * 0.5f };
		break;
	}
}

void TransformComponent::Update(float deltaTime)
{
	constexpr float moveSpeed = 100.0f; // 초당 이동 속도
	D2D1_POINT_2F delta = { 0.0f, 0.0f };

	if (m_IsWPressed) delta.y += moveSpeed * deltaTime;
	if (m_IsAPressed) delta.x -= moveSpeed * deltaTime;
	if (m_IsSPressed) delta.y -= moveSpeed * deltaTime;
	if (m_IsDPressed) delta.x += moveSpeed * deltaTime;

	m_Position.x += delta.x;
	m_Position.y += delta.y;

	if (delta.x != 0 || delta.y != 0)
		SetDirty();
}

void TransformComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::KeyDown)
	{
		auto keyData = static_cast<const Events::KeyEvent*>(data);
		if (!keyData) return;

		bool isDown = (type == EventType::KeyDown);

		switch (keyData->key)
		{
		case 'W': m_IsWPressed = isDown; break;
		case 'A': m_IsAPressed = isDown; break;
		case 'S': m_IsSPressed = isDown; break;
		case 'D': m_IsDPressed = isDown; break;
		default: break;
		}
	}
	else
	{
		auto keyData = static_cast<const Events::KeyEvent*>(data);
		if (!keyData) return;

		bool isDown = (type == EventType::KeyDown);

		switch (keyData->key)
		{
		case 'W': m_IsWPressed = isDown; break;
		case 'A': m_IsAPressed = isDown; break;
		case 'S': m_IsSPressed = isDown; break;
		case 'D': m_IsDPressed = isDown; break;
		default: break;
		}
	}
}

void TransformComponent::Serialize(nlohmann::json& j) const
{
	j["position"]["x"] = m_Position.x;
	j["position"]["y"] = m_Position.y;
	j["rotation"] = m_Rotation;
	j["scale"]["x"] = m_Scale.x;
	j["scale"]["y"] = m_Scale.y;
	j["scale"]["y"] = m_Scale.y;
}

void TransformComponent::Deserialize(const nlohmann::json& j)
{
	m_Position.x = j["position"]["x"].get<float>();
	m_Position.y = j["position"]["y"].get<float>();
	m_Rotation = j["rotation"].get<float>();
	m_Scale.x = j["scale"]["x"].get<float>();
	m_Scale.y = j["scale"]["y"].get<float>();
	UpdateMatrices();
}

void TransformComponent::UpdateMatrices()
{
	auto mat_scale = Matrix3X2F::Scale(m_Scale.x, m_Scale.y, D2D1::Point2F(m_Pivot.x, m_Pivot.y));

	auto mat_rot = Matrix3X2F::Rotation(m_Rotation, D2D1::Point2F(m_Pivot.x, m_Pivot.y));

	auto mat_trans = Matrix3X2F::Translation(m_Position.x, m_Position.y);

	m_LocalMatrix = mat_scale * mat_rot * mat_trans;

	if (m_Parent)
	{
		m_WorldMatrix = m_LocalMatrix * m_Parent->GetWorldMatrix();
	}
	else
	{
		m_WorldMatrix = m_LocalMatrix;
	}

	m_IsDirty = false;
}
