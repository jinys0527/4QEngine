
#include "NodeComponent.h"
#include "ReflectionMacro.h"
#include "MaterialComponent.h"
#include "Object.h"
#include "Event.h"

REGISTER_COMPONENT(NodeComponent)
REGISTER_PROPERTY(NodeComponent, IsMoveable)
REGISTER_PROPERTY_READONLY(NodeComponent, StateInt)
REGISTER_PROPERTY_READONLY(NodeComponent, Q)
REGISTER_PROPERTY_READONLY(NodeComponent, R)

NodeComponent::NodeComponent() = default;

NodeComponent::~NodeComponent() {
	// Event Listener 쓰는 경우만
	/*GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);*/
}


void NodeComponent::Start()
{
	// 이벤트 추가
	/*GetEventDispatcher().AddListener(EventType::KeyDown, this);
	GetEventDispatcher().AddListener(EventType::KeyUp, this);*/
	auto* owner = GetOwner();
	if (owner)
	{
		m_Material = owner->GetComponent<MaterialComponent>();
		if (m_Material)
		{
			m_BaseMaterialOverrides = m_Material->GetOverrides();
			m_HasBaseMaterial = true;
		}
	}
}


void NodeComponent::Update(float deltaTime) {

	switch (m_State)
	{
	case NodeState::Empty : m_StateInt = 0; break;
	case NodeState::HasPlayer: m_StateInt = 1; break;
	case NodeState::HasEnemy: m_StateInt = 2; break;
	}
}



void NodeComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}



void NodeComponent::ClearNeighbors()
{
	m_Neighbors.clear();
}



void NodeComponent::AddNeighbor(NodeComponent* node)
{
	if (!node) {
		return; 
	}
	m_Neighbors.push_back(node);
}

// 강조 표시
void NodeComponent::SetMoveRangeHighlight(float intensity, bool enabled)
{
	if (!m_Material)
	{
		return;
	}

	if (!m_HasBaseMaterial)
	{
		m_BaseMaterialOverrides = m_Material->GetOverrides();
		m_HasBaseMaterial = true;
	}

	if (!enabled)
	{
		if (m_UsingMoveRangeHighlight)
		{
			m_Material->SetOverrides(m_BaseMaterialOverrides);
			m_UsingMoveRangeHighlight = false;
		}
		return;
	}

	const float clampedIntensity = std::clamp(intensity, 0.0f, 1.0f);
	RenderData::MaterialData overrides = m_BaseMaterialOverrides;
	const auto& baseColor = m_BaseMaterialOverrides.baseColor;
	const DirectX::XMFLOAT4 highlightColor{ 0.2f, 1.0f, 0.2f, baseColor.w };

	overrides.baseColor.x = baseColor.x + (highlightColor.x - baseColor.x) * clampedIntensity;
	overrides.baseColor.y = baseColor.y + (highlightColor.y - baseColor.y) * clampedIntensity;
	overrides.baseColor.z = baseColor.z + (highlightColor.z - baseColor.z) * clampedIntensity;
	overrides.baseColor.w = baseColor.w;

	m_Material->SetOverrides(overrides);
	m_UsingMoveRangeHighlight = true;
}
