#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "GridSystemComponent.h"
#include <cmath>

REGISTER_COMPONENT(PlayerComponent)
REGISTER_PROPERTY_READONLY(PlayerComponent, Q)
REGISTER_PROPERTY_READONLY(PlayerComponent, R)
REGISTER_PROPERTY(PlayerComponent, MoveResource)
REGISTER_PROPERTY(PlayerComponent, ActResource)
REGISTER_PROPERTY(PlayerComponent, PlayerTurnTime)
REGISTER_PROPERTY_READONLY(PlayerComponent, TurnElapsed)
REGISTER_PROPERTY_READONLY(PlayerComponent, RemainMoveResource)


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
}

void PlayerComponent::Start()
{
	//start시 GameManagaer get
	ResetTurnResources();
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene) { return; }

	auto& service = scene->GetServices();
	if (service.Has<GameManager>()) {
		m_GameManager = &service.Get<GameManager>();
		m_LastTurn = m_GameManager->GetTurn();
	}
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
	if (!scene)
	{
		return;
	}

	if (!m_GameManager)
	{
		auto& services = scene->GetServices();
		if (services.Has<GameManager>())
		{
			m_GameManager = &services.Get<GameManager>();
		}
	}
	if (!m_GameManager)
	{
		return;
	}
	//

	const auto currentTurn = m_GameManager->GetTurn();
	if (currentTurn != m_LastTurn)
	{
		m_LastTurn = currentTurn;
		if (currentTurn == Turn::PlayerTurn)
		{
			ResetTurnResources();
		}
		// turn 종류에 따른 행동정의
	}


	//Player Turn 종료 조건

	if (currentTurn == Turn::PlayerTurn) {
		m_TurnElapsed += deltaTime;

		// 조건 추가(행동력)
		if (m_RemainMoveResource <= 0 || m_TurnElapsed >= m_PlayerTurnTime) {
			//종료(턴 전환)
			m_GameManager->SetTurn(Turn::EnemyTurn);
		}
	}
}

void PlayerComponent::OnEvent(EventType type, const void* data)
{


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
