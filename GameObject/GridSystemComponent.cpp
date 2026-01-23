/// 만들기 귀찮아서 만들어둔 Template(c++ Template 아님)
/// 복붙해서 쓰세요
#include "GridSystemComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "NodeComponent.h"
#include "GameObject.h"

REGISTER_COMPONENT(GridSystemComponent)
REGISTER_PROPERTY_READONLY(GridSystemComponent, NodesCount)

GridSystemComponent::GridSystemComponent() {

}

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
	if (!GetOwner()) { return; }
	auto* scene = GetOwner()->GetScene();
	if (!scene) { return; }

	const auto& objects = scene->GetGameObjects();
	m_Nodes.reserve(objects.size());

	//등록 과정
	for (const auto& [name, object] : objects) {
		if(!object){ continue;}

		auto* node = object->GetComponent<NodeComponent>();
		if(!node) { continue;}

		m_Nodes.push_back(node);
		m_NodesCount++; // debuging 용
	}

}

void GridSystemComponent::MakeGraph()
{
	for (auto* node : m_Nodes)
	{
		if (!node)
		{
			continue;
		}

		node->ClearNeighbors();
	}

}

