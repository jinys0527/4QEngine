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

	void SetIsMoveable(const bool& is) { m_IsMoveable = is;  };
	const bool& GetIsMoveable() const { return m_IsMoveable; }

	void SetQR(int q, int r) { m_Q = q; m_R = r;  }
	

	const int& GetQ() const { return m_Q; }
	const int& GetR() const { return m_R; }
	
	//상태 발판 설정
	void SetState(NodeState state) { m_State = state; }
	NodeState GetState() const { return m_State; }

	const int& GetStateInt() const { return m_StateInt; } // Debug

	void ClearNeighbors();
	void AddNeighbor(NodeComponent* node);
	const vector<NodeComponent*>& GetNeighbors() const { return m_Neighbors; }
private:

	bool m_IsMoveable = true; //장애물 있으면 Editor에서 배치할때 false로 설정하기
	//Read Only Property
	NodeState m_State = NodeState::Empty;
	int m_Q; 
	int m_R;
	int m_StateInt = 0; // Debug
	// Property X
	vector<NodeComponent*> m_Neighbors;
};