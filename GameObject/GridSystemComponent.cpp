
#include <cmath>
#include <unordered_map>
#include "GridSystemComponent.h"
#include "TransformComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "NodeComponent.h"
#include "GameObject.h"

REGISTER_COMPONENT(GridSystemComponent)
REGISTER_PROPERTY_READONLY(GridSystemComponent, NodesCount)

using namespace std;

// 면 갯수 / Index
constexpr int kNeighborCount = 6;
constexpr int kNeighborOffsets[kNeighborCount][2] = {
	{ 1, 0 },
	{ 1, -1 },
	{ 0, -1 },
	{ -1, 0 },
	{ -1, 1 },
	{ 0, 1 }
};
// unordered_map key로 사용할 것임

float ClampInnerRadius(float innerRadius)
{
	return (innerRadius > 0.0f) ? innerRadius : 1.0f;
}

float OuterRadiusFromInner(float innerRadius)
{
	return ClampInnerRadius(innerRadius) * 2.0f / std::sqrt(3.0f);
}

AxialCoord WorldToAxialPointy(const XMFLOAT3& pos, float innerRadius)
{
	const float size = OuterRadiusFromInner(innerRadius);
	const float invSize = (size > 0.0f) ? (1.0f / size) : 0.0f;
	const float x = pos.x;
	const float z = pos.z;
	return {
		(std::sqrt(3.0f) / 3.0f * x - 1.0f / 3.0f * z) * invSize,
		(2.0f / 3.0f * z) * invSize
	};
}

// HexaGrid 좌표의 정합성(3개의 합 0 유지) Cube -> Axial
AxialKey AxialRound(const AxialCoord& axial)
{
	CubeCoord cube{ axial.q, -axial.q - axial.r, axial.r };

	float rx = std::round(cube.x);
	float ry = std::round(cube.y);
	float rz = std::round(cube.z);

	const float dx = std::fabs(rx - cube.x);
	const float dy = std::fabs(ry - cube.y);
	const float dz = std::fabs(rz - cube.z);

	if (dx > dy && dx > dz)
	{
		rx = -ry - rz;
	}
	else if (dy > dz)
	{
		ry = -rx - rz;
	}
	else
	{
		rz = -rx - ry;
	}

	return { static_cast<int>(rx), static_cast<int>(rz) };
}

GridSystemComponent::GridSystemComponent() = default;

GridSystemComponent::~GridSystemComponent() {
	// Event Listener 쓰는 경우만
	/*GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);*/
}

void GridSystemComponent::Start()
{
	// 이벤트 추가
	/*GetEventDispatcher().AddListener(EventType::KeyDown, this);
	GetEventDispatcher().AddListener(EventType::KeyUp, this);*/

	ScanNodes();
	MakeGraph();
}

void GridSystemComponent::Update(float deltaTime) {
	(void)deltaTime;
}

void GridSystemComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void GridSystemComponent::ScanNodes()
{
	m_Nodes.clear();
	m_NodesByAxial.clear();
	if (!GetOwner()) { return; }
	auto* scene = GetOwner()->GetScene();
	if (!scene) { return; }

	const auto& objects = scene->GetGameObjects();

	// 메모리를 불필요하게 잡는 것일 수 있음 필요하면 최적화
	m_Nodes.reserve(objects.size()); 
	m_NodesByAxial.reserve(objects.size());


	//등록 과정 ( Transform 기반 Axial 좌표변환 후 등록)
	for (const auto& [name, object] : objects) {
		if(!object){ continue;}

		auto* node = object->GetComponent<NodeComponent>();
		if(!node) { continue; }
		auto* trans = object->GetComponent<TransformComponent>();

		const AxialKey axial = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));


		node->SetQR(axial.q, axial.r);

		m_Nodes.push_back(node);
		m_NodesByAxial[{ axial.q, axial.r }] = node;

		m_NodesCount++; // debuging 용ㅤ
	}

}

void GridSystemComponent::MakeGraph()
{
	//unordered_map<AxialKey, NodeComponent*, AxialKeyHash> nodesByAxial;

	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}

		node->ClearNeighbors();
	}

	// 모든 노드들에 대하여 인접 노드 등록 (Hash된 QRKey 로)
	for (auto* node : m_Nodes) {

		const int q = node->GetQ();
		const int r = node->GetR();

		for (int i = 0; i < kNeighborCount; i++) {

			const AxialKey key{ q + kNeighborOffsets[i][0], r + kNeighborOffsets[i][1] };

			auto it = m_NodesByAxial.find(key);
			if (it != m_NodesByAxial.end()) {
				node->AddNeighbor(it->second);
			}
		}
	}

}

