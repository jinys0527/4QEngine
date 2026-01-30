#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "Scene.h"
#include "GridSystemComponent.h"
#include "ItemComponent.h"
#include "SkeletalMeshComponent.h"
#include "TransformComponent.h"
#include <cmath>

REGISTER_COMPONENT(PlayerComponent)
REGISTER_PROPERTY_READONLY(PlayerComponent, Q)
REGISTER_PROPERTY_READONLY(PlayerComponent, R)
REGISTER_PROPERTY(PlayerComponent, MoveResource)
REGISTER_PROPERTY(PlayerComponent, ActResource)
REGISTER_PROPERTY(PlayerComponent, PlayerTurnTime)
REGISTER_PROPERTY_READONLY(PlayerComponent, TurnElapsed)
REGISTER_PROPERTY_READONLY(PlayerComponent, RemainMoveResource)

//REGISTER_PROPERTY(PlayerComponent, Item)

static int AxialDistance(int q1, int r1, int q2, int r2)
{
	const int dq = q1 - q2;
	const int dr = r1 - r2;
	const int ds = dq + dr;
	return (std::abs(dq) + std::abs(dr) + std::abs(ds)) / 2;
}


PlayerComponent::PlayerComponent() {

}

PlayerComponent::~PlayerComponent() {
	// Event Listener 쓰는 경우만	
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
}

void PlayerComponent::Start()
{
	//start시 GameManagaer get
	ResetTurnResources();
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene) { return; }

	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
	const auto& objects = scene->GetGameObjects();

	for (const auto& [name,object] : objects) {
		if (!object) { continue; }
		
		if (auto* grid = object->GetComponent<GridSystemComponent>()) {
			m_GridSystem = grid;
			break;
		}
	}
}

void PlayerComponent::Update(float deltaTime) {
	(void)deltaTime;

	//defense
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene ||scene->GetIsPause())
	{
		return;
	}

	//Player Turn 종료 조건

	if (m_CurrentTurn == Turn::PlayerTurn) {
		m_TurnElapsed += deltaTime;

		if (!m_TurnEndRequested && m_TurnElapsed >= m_PlayerTurnTime) {
			//종료(턴 전환)
			GetEventDispatcher().Dispatch(EventType::PlayerTurnEndRequested, nullptr);
			m_TurnEndRequested = true;
		}
	}

	auto* transformcomponent = owner->GetComponent<TransformComponent>();
	{
		if (!transformcomponent->GetChildrens().empty())
		{
			m_Item = dynamic_cast<GameObject*>(transformcomponent->GetChildrens()[0]->GetOwner());
		}

	}

	//아이템이 있으면 그 아이템에서 장착 본 행렬 넘겨주기
	//스켈레탈이 있으면 장착 본 행렬을 RenderData에 넘겨주기
	if (m_Item != nullptr)
	{
		auto* itemcomponent = m_Item->GetComponent<ItemComponent>();

		auto* skeletal = owner->GetComponent<SkeletalMeshComponent>();
		if (!skeletal)
		{
			return;
		}

		auto* loader = AssetLoader::GetActive();
		if (!loader)
		{
			return;
		}

		const SkeletonHandle skeletonHandle = skeletal->GetSkeletonHandle();
		if (!skeletonHandle.IsValid())
		{
			return;
		}

		RenderData::Skeleton* skeleton = loader->GetSkeletons().Get(skeletonHandle);
		if (!skeleton)
		{
			return;
		}

		XMFLOAT4X4 mtm = skeleton->equipmentBindPose;

		itemcomponent->SetEquipmentBindPose(skeleton->equipmentBindPose);
		int a = 0;
	}
}

void PlayerComponent::OnEvent(EventType type, const void* data)
{
	if (type != EventType::TurnChanged || !data)
	{
		return;
	}

	const auto* payload = static_cast<const Events::TurnChanged*>(data);
	if (!payload)
	{
		return;
	}

	m_CurrentTurn = static_cast<Turn>(payload->turn);
	m_TurnElapsed = 0.0f;
	m_TurnEndRequested = false;
	if (m_CurrentTurn == Turn::PlayerTurn)
	{
		ResetTurnResources();
	}
}

// 행동,이동력 초기화 // turn 초기화
void PlayerComponent::ResetTurnResources()
{
	m_RemainMoveResource = m_MoveResource;
	m_RemainActResource = m_ActResource;
	m_TurnElapsed = 0.0f;
	m_HasMoveStart = false;
}

// 움직임
void PlayerComponent::BeginMove()
{
	m_StartQ = m_Q;
	m_StartR = m_R;
	m_HasMoveStart = true;
}

bool PlayerComponent::CommitMove(int targetQ, int targetR)
{
	const int startQ = m_HasMoveStart ? m_StartQ : m_Q;
	const int startR = m_HasMoveStart ? m_StartR : m_R;
	//
	int cost = -1;
	if (m_GridSystem) {
		cost = m_GridSystem->GetShortestPathLength({ startQ,startR }, { targetQ,targetR });
	}
	if (cost < 0) {
		const int cost = AxialDistance(startQ, startR, targetQ, targetR); // start와 Target 간의 소모 cost return
	}
	m_HasMoveStart = false;
	if (cost <= 0)
	{
		return true;
	}
	if (cost > m_RemainMoveResource) //남은 MoveResource 보다 크면 Commit X 
	{
		return false;
	}

	m_RemainMoveResource -= cost; //반영
	return true;

}

bool PlayerComponent::ConsumeActResource(int amount)
{
	if (amount <= 0)
	{
		return true;
	}
	if (amount > m_RemainActResource)
	{
		return false;
	}
	m_RemainActResource -= amount;
	return true;
}
