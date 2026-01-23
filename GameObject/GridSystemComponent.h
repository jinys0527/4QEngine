#pragma once
/// 만들기 귀찮아서 만들어둔 Template(c++ Template 아님)
/// 복붙해서 이름 바꿔서 쓰세요
#include "Component.h"
#include "NodeComponent.h"
using namespace std;
// 이벤트 리스너는 쓸 얘들만
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

private:

	void ScanNodes(); // Scene 순회 후 Nodes 등록
	void MakeGraph();// 위치기반 노드 연결
	// node 받기
	vector<NodeComponent*> m_Nodes;
	int m_NodesCount = 0;
};