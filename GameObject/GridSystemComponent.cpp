
#include <cmath>
#include <queue>
#include <unordered_map>
#include "GridSystemComponent.h"
#include "TransformComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "NodeComponent.h"
#include "GameObject.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"

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
}

void GridSystemComponent::Start()
{

	ScanNodes();
	MakeGraph();
}

void GridSystemComponent::Update(float deltaTime) {
	(void)deltaTime;

	UpdateActorPositions();
	const int moveRange = m_Player ? m_Player->GetRemainMoveResource() : 0; // 남은 이동 자원 Get
	UpdateMoveRange(m_PlayerNode, moveRange);
}

void GridSystemComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

//Node object 찾아서 배정
void GridSystemComponent::ScanNodes()
{
	m_NodesCount = 0;
	m_Nodes.clear();
	m_NodesByAxial.clear();
	m_Player = nullptr;
	m_Enemies.clear();


	if (!GetOwner()) { return; }
	auto* scene = GetOwner()->GetScene();
	if (!scene) { return; }

	const auto& objects = scene->GetGameObjects();

	// 메모리를 불필요하게 잡는 것일 수 있음 필요하면 최적화
	m_Nodes.reserve(objects.size()); 
	m_NodesByAxial.reserve(objects.size());


	//등록 과정 ( Transform 기반 Axial 좌표변환 후 등록) ** Enemy, Player 도 추가
	for (const auto& [name, object] : objects) {
		if(!object){ continue;}

		auto* node = object->GetComponent<NodeComponent>();

		if(!node) { 
			auto* enemy =  object->GetComponent<EnemyComponent>();
			if (!enemy) {
				//Player 처리
				auto* player = object->GetComponent<PlayerComponent>();
				if (!player) { continue; }
				auto* trans = object->GetComponent<TransformComponent>();
				const AxialKey axial = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));
				player->SetQR(axial.q, axial.r);
				m_Player = player;
				continue;
			}
			//Enemy 처리
			auto* trans = object->GetComponent<TransformComponent>();
			const AxialKey axial = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));
			enemy->SetQR(axial.q, axial.r);
			m_Enemies.push_back(enemy);
			continue;
		}

		// node처리
		auto* trans = object->GetComponent<TransformComponent>();
		const AxialKey axial = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));


		node->SetQR(axial.q, axial.r);
		node->SetState(NodeState::Empty); //?

		m_Nodes.push_back(node);
		m_NodesByAxial[{ axial.q, axial.r }] = node;

		m_NodesCount++; // debuging 용ㅤ
	}

	if (m_Player) {
		const AxialKey current{ m_Player->GetQ(), m_Player->GetR() };
		UpdateActorNodeState(current, current, NodeState::HasPlayer);
	}

	for (auto* enemy : m_Enemies) {
		if (!enemy) {
			continue;
		}
		const AxialKey current{ enemy->GetQ(), enemy->GetR() };
		UpdateActorNodeState(current, current, NodeState::HasEnemy);
	}

}

// Axial 좌표로 Node 찾기
NodeComponent* GridSystemComponent::GetNodeByKey(const AxialKey& key) const
{
	auto it = m_NodesByAxial.find(key);
	if (it == m_NodesByAxial.end())
	{
		return nullptr;
	}
	return it->second;
}

void GridSystemComponent::UpdateMoveRange(NodeComponent* startNode, int range)
{
	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}
		node->SetInMoveRange(false);
	}

	if (!startNode || range <= 0)
	{
		return;
	}

	std::unordered_map<NodeComponent*, int> distances;
	std::queue<NodeComponent*> frontier;

	distances[startNode] = 0;
	frontier.push(startNode);
	startNode->SetInMoveRange(true);

	while (!frontier.empty())
	{
		NodeComponent* current = frontier.front();
		frontier.pop();

		const int currentDistance = distances[current];
		if (currentDistance >= range)
		{
			continue;
		}

		for (auto* neighbor : current->GetNeighbors())
		{
			if (!neighbor)
			{
				continue;
			}

			if (neighbor->GetState() != NodeState::Empty)
			{
				continue;
			}

			if (!neighbor->GetIsMoveable())
			{
				continue;
			}

			if (distances.find(neighbor) != distances.end())
			{
				continue;
			}

			const int nextDistance = currentDistance + 1;
			distances[neighbor] = nextDistance;
			neighbor->SetInMoveRange(true);
			frontier.push(neighbor);
		}
	}
}

//Player/ Enemy 변경에 따른 Node state Update
void GridSystemComponent::UpdateActorPositions() 
{
	if (m_Player) {
		auto* playerOwner = m_Player->GetOwner();
		auto* trans = playerOwner ? playerOwner->GetComponent<TransformComponent>() : nullptr;
		if (trans) {
			const AxialKey previous{ m_Player->GetQ(), m_Player->GetR() };
			const AxialKey current = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));
			if (!(previous == current)) {
				UpdateActorNodeState(previous, current, NodeState::HasPlayer);
				m_Player->SetQR(current.q, current.r);
			}
		}
	}

	for (auto* enemy : m_Enemies) {
		if (!enemy) {
			continue;
		}
		auto* enemyOwner = enemy->GetOwner();
		auto* trans = enemyOwner ? enemyOwner->GetComponent<TransformComponent>() : nullptr;
		if (!trans) {
			continue;
		}
		const AxialKey previous{ enemy->GetQ(), enemy->GetR() };
		const AxialKey current = AxialRound(WorldToAxialPointy(trans->GetPosition(), m_InnerRadius));
		if (!(previous == current)) {
			UpdateActorNodeState(previous, current, NodeState::HasEnemy);
			enemy->SetQR(current.q, current.r);
		}
	}
}

void GridSystemComponent::UpdateActorNodeState(const AxialKey& previous, const AxialKey& current, NodeState state)
{
	if (!(previous == current)) {
		auto it = m_NodesByAxial.find(previous);
		if (it != m_NodesByAxial.end() && it->second) {
			if (it->second->GetState() == state) {
				it->second->SetState(NodeState::Empty);
			}
		}
	}

	auto it = m_NodesByAxial.find(current);
	if (it != m_NodesByAxial.end() && it->second) {
		it->second->SetState(state);
		if (state == NodeState::HasPlayer) {
			m_PlayerNode = it->second;
		}
	}
	else if (state == NodeState::HasPlayer) {
		m_PlayerNode = nullptr;
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

