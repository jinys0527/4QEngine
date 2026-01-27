#pragma once
#include "Component.h"
#include "NodeComponent.h"
using namespace std;

class PlayerComponent;
class EnemyComponent;

struct AxialKey {
	int q = 0;
	int r = 0;
	bool operator ==(const AxialKey& other) const { return q == other.q && r == other.r; }
};

// AxialKey 는 struct, 커스텀 해시 필요
struct AxialKeyHash {
	size_t operator() (const AxialKey& key) const noexcept {
		const size_t hq = hash<int>{} (key.q);
		const size_t hr = hash<int>{} (key.r);
		return hq ^ (hr + 0x9e3779b9 + (hq << 6) + (hq >> 2)); // Hasing 연산 //magic number
	}
};

struct CubeCoord {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
};

struct AxialCoord {
	float q = 0.0f;
	float r = 0.0f;
};


class GridSystemComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "GridSystemComponent";
	const char* GetTypeName() const override;

	GridSystemComponent();
	virtual ~GridSystemComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요
	const vector<NodeComponent*>& GetNodes() const { return m_Nodes; }
	const int& GetNodesCount() const { return m_NodesCount; }
	int GetShortestPathLength(const AxialKey& start,const AxialKey& target);
	NodeComponent* GetNodeByKey(const AxialKey& key) const;

private:

	void ScanNodes(); // Scene 순회 후 Nodes 등록
	void MakeGraph();// 위치기반 노드 연결
	void UpdateMoveRange(NodeComponent* startNode, int range);
	void UpdateActorPositions();
	void UpdateActorNodeState(const AxialKey& previous, const AxialKey& current, NodeState state);
	

	void CalculatePath(); // 길찾기
	// node 받기
	vector<NodeComponent*> m_Nodes;
	vector<EnemyComponent*> m_Enemies;
	NodeComponent* m_PlayerNode = nullptr;
	PlayerComponent* m_Player = nullptr;

	std::unordered_map<AxialKey, NodeComponent*, AxialKeyHash> m_NodesByAxial;
	int m_NodesCount = 0; //for Debug

	float m_InnerRadius = 0.866f; // 리소스 바뀌면 1로 변경 (현재는 외접기준 1)
};