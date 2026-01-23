#pragma once
#include <vector>
#include "Component.h"
#include "MathHelper.h"

using namespace MathUtils;
using namespace std; 
// 노드 상태
enum class NodeState {
	Empty,
	HasPlayer,
	HasMonster,
	//HasObstacle, -> m_IsMoveable = false 로 고정 설정 할것이기 때문에 제외 
};


class NodeComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "NodeComponent";
	const char* GetTypeName() const override;

	NodeComponent();
	virtual ~NodeComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; 

	void SetIsMoveable(const float& is) { m_IsMoveable = is;  };
	const bool& GetIsMoveable() const { return m_IsMoveable; }

	const XMFLOAT3& GetNodePos() const { return m_pos; }
	NodeState GetState() const { return m_State; }

	void ClearNeighbors();
	void AddNeighbor(NodeComponent* node);
	const vector<NodeComponent*>& GetNeighbors() const { return m_Neighbors; }
private:

	bool m_IsMoveable = true; //장애물 있으면 Editor에서 배치할때 false로 설정하기

	//Read Only Property
	NodeState m_State = NodeState::Empty;

	// Property 노출 X
	XMFLOAT3 m_pos = { 0.0f,0.0f,0.0f };
	
	vector<NodeComponent*> m_Neighbors;
	//
};