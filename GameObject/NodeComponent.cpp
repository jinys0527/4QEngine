
#include "NodeComponent.h"
#include "ReflectionMacro.h"
#include "TransformComponent.h"
#include "Object.h"
#include "Event.h"

REGISTER_COMPONENT(NodeComponent)

NodeComponent::NodeComponent() {

}

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

	// Node는 무조건 Transform 갖음
	if (!GetOwner())
	{
		return;
	}
	auto transComp = GetOwner()->GetComponent<TransformComponent>();
	if (transComp) {
		XMFLOAT3 m_pos = transComp->GetPosition();
	}

}


void NodeComponent::Update(float deltaTime) {

	(void)deltaTime;
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
