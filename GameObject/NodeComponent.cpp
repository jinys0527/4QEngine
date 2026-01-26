
#include "NodeComponent.h"
#include "ReflectionMacro.h"
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

}


void NodeComponent::Update(float deltaTime) {

	switch (m_State)
	{
	case NodeState::Empty : m_StateInt = 0; break;
	case NodeState::HasPlayer: m_StateInt = 1; break;
	case NodeState::HasMonster: m_StateInt = 2; break;
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
