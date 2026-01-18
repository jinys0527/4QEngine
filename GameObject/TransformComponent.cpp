#include "TransformComponent.h"
#include "Event.h"
#include <cassert>
#include "ReflectionMacro.h"
REGISTER_COMPONENT(TransformComponent)  //컴포넌트 등록
// 컴포넌트 property 등록 **이름 주의**
REGISTER_PROPERTY(TransformComponent, Position)
REGISTER_PROPERTY(TransformComponent, Rotation)
REGISTER_PROPERTY(TransformComponent, Scale)


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
	XMFLOAT4X4 childLocalTM = child->GetLocalMatrix();
	childLocalTM = Mul(childLocalTM, GetInverseWorldMatrix());

	XMFLOAT4X4 mNoPivot = RemovePivot(childLocalTM, child->GetPivotPoint());
	DecomposeMatrix(mNoPivot, child->m_Position, child->m_Rotation, child->m_Scale);

	m_Children.push_back(child);
}

void TransformComponent::RemoveChild(TransformComponent* child)
{
	XMFLOAT4X4 childLocalTM = child->GetLocalMatrix();
	childLocalTM = Mul(childLocalTM, m_WorldMatrix);

	XMFLOAT4X4 mNoPivot = RemovePivot(childLocalTM, child->GetPivotPoint());
	DecomposeMatrix(mNoPivot, child->m_Position, child->m_Rotation, child->m_Scale);

	m_Children.erase(
		std::remove(m_Children.begin(), m_Children.end(), child),
			m_Children.end()
	);
}

void TransformComponent::SetRotationEuler(const XMFLOAT3& rot)
{	// 쿼터니언 -> 오일러 변환
	// 변환 순서
	// degree -> radian -> Quater

	XMFLOAT3 rotRad{ XMConvertToRadians(rot.x),XMConvertToRadians(rot.y),XMConvertToRadians(rot.z) };
		

	XMFLOAT4 result{};
	XMStoreFloat4(&result, XMQuaternionRotationRollPitchYaw(rotRad.x, rotRad.y, rotRad.z));
	m_Rotation = result;
	SetDirty();
}

void TransformComponent::Translate(const XMFLOAT3& delta)
{
	AddInPlace(m_Position, delta);

	SetDirty();
}

void TransformComponent::Translate(const float x, const float y, const float z)
{
	XMFLOAT3 delta = { x, y, z };
	AddInPlace(m_Position, delta);

	SetDirty();
}

void TransformComponent::Rotate(float pitch, float yaw, float roll)
{
	XMFLOAT4 quat = QuatFromEular(pitch, yaw, roll);
	MulInPlace(m_Rotation, quat);
}

void TransformComponent::Rotate(const XMFLOAT4& rot)
{
	MulInPlace(m_Rotation, rot);
}

XMFLOAT3 TransformComponent::GetForward() const
{
	XMFLOAT3 forward;
	XMVECTOR vForward = XMVector3Rotate(
		XMVectorSet(0.f, 0.f, 1.f, 0.f),
		XMLoadFloat4(&m_Rotation)
	);
	XMStoreFloat3(&forward, vForward);

	return forward;
}

const XMFLOAT4X4& TransformComponent::GetWorldMatrix() 
{
	if (m_IsDirty) UpdateMatrices();
	return m_WorldMatrix;
}

const XMFLOAT4X4& TransformComponent::GetLocalMatrix()
{
	if (m_IsDirty) UpdateMatrices();
	return m_LocalMatrix;
}

XMFLOAT4X4 TransformComponent::GetInverseWorldMatrix()
{
	XMFLOAT4X4 inv = Inverse(m_WorldMatrix);
	return inv;
}

void TransformComponent::SetPivotPreset(TransformPivotPreset preset, const XMFLOAT3& size)
{
	switch (preset)
	{
	case TransformPivotPreset::BottomCenter:
		m_Pivot = { 0, -1, 0 };
		break;
	case TransformPivotPreset::Center:
		m_Pivot = { 0, 0, 0 };
		break;
	case TransformPivotPreset::BoundingPivot:
		break;
	}
}

void TransformComponent::Update(float deltaTime)
{
}

void TransformComponent::OnEvent(EventType type, const void* data)
{
}


void TransformComponent::Deserialize(const nlohmann::json& j)
{
	/*m_Position.x = j["position"]["x"];
	m_Position.y = j["position"]["y"];
	m_Position.z = j["position"]["z"];

	m_Rotation.x = j["rotation"]["x"];
	m_Rotation.y = j["rotation"]["y"];
	m_Rotation.z = j["rotation"]["z"];
	m_Rotation.w = j["rotation"]["w"];

	m_Scale.x = j["scale"]["x"];
	m_Scale.y = j["scale"]["y"];
	m_Scale.z = j["scale"]["z"];*/
	Component::Deserialize(j);
	UpdateMatrices();
}

void TransformComponent::UpdateMatrices()
{
	XMStoreFloat4x4(&m_LocalMatrix, CreateTRS(m_Position, m_Rotation, m_Scale));

	if (m_Parent)
	{
		m_WorldMatrix = Mul(m_LocalMatrix, m_Parent->m_WorldMatrix);
	}
	else
	{
		m_WorldMatrix = m_LocalMatrix;
	}

	m_IsDirty = false;
}
