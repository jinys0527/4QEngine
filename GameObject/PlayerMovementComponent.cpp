#include "PlayerMovementComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Event.h"
#include "TransformComponent.h"
#include "ServiceRegistry.h"
#include "InputManager.h"
#include "Scene.h"
#include "RayHelper.h"
#include "CameraObject.h"
#include "BoxColliderComponent.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "GridSystemComponent.h"
#include "PlayerMoveFSMComponent.h"
#include "PlayerFSMComponent.h"
#include "GameState.h"
#include <array>
//#include <cfloat>

REGISTER_COMPONENT(PlayerMovementComponent)
REGISTER_PROPERTY(PlayerMovementComponent, DragSpeed)

static NodeComponent* FindClosestNodeHit(
	Scene* scene,
	const DirectX::XMFLOAT3& rayOrigin,
	const DirectX::XMFLOAT3& rayDir,
	float& outT)
{
	if (!scene) return nullptr;

	auto& gameObjects = scene->GetGameObjects();

	float closestT = FLT_MAX;
	NodeComponent* closestNode = nullptr;

	for (const auto& [name, object] : gameObjects)
	{
		if (!object) continue;

		auto* node = object->GetComponent<NodeComponent>();
		if (!node) continue;

		auto* col = object->GetComponent<BoxColliderComponent>();
		if (!col || !col->HasBounds()) continue;

		float t = 0.0f;
		if (!col->IntersectsRay(rayOrigin, rayDir, t)) continue;

		if (t >= 0.0f && t < closestT)
		{
			closestT = t;
			closestNode = node;
		}
	}

	if (!closestNode)
		return nullptr;

	outT = closestT;
	return closestNode;
}

static bool TryGetRotationFromStep(const AxialKey& previous, const AxialKey& current, RotationOffset& outDir)
{
	const AxialKey delta{ current.q - previous.q, current.r - previous.r };
	constexpr std::array<std::pair<AxialKey, RotationOffset>, 6> kDirections{ {
		{ { 1, 0 }, RotationOffset::clock_3 },
		{ { 1, -1 }, RotationOffset::clock_5 },
		{ { 0, -1 }, RotationOffset::clock_7 },
		{ { -1, 0 }, RotationOffset::clock_9 },
		{ { -1, 1 }, RotationOffset::clock_11 },
		{ { 0, 1 }, RotationOffset::clock_1 }
	} };


	for (const auto& [dir, rotation] : kDirections)
	{
		if (dir.q == delta.q && dir.r == delta.r)
		{
			outDir = rotation;
			return true;
		}
	}

	return false;
}
namespace
{
	void DispatchPlayerStateEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerFSMComponent>())
			fsm->DispatchEvent(eventName);
	}

	void DispatchMoveEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerMoveFSMComponent>())
			fsm->DispatchEvent(eventName);
	}
}

PlayerMovementComponent::PlayerMovementComponent()
{
}

PlayerMovementComponent::~PlayerMovementComponent()
{
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().RemoveListener(EventType::Dragged, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClickUp, this);
	GetEventDispatcher().RemoveListener(EventType::MouseRightClick, this);
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
}

void PlayerMovementComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::KeyDown, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().AddListener(EventType::Dragged, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClickUp, this);
	GetEventDispatcher().AddListener(EventType::MouseRightClick, this);
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
		return;

	const auto& objects = scene->GetGameObjects();
	for (const auto& [name, object] : objects)
	{
		if (!object)
			continue;

		if (auto* grid = object->GetComponent<GridSystemComponent>())
		{
			m_GridSystem = grid;
			break;
		}
	}
}

void PlayerMovementComponent::Update(float deltaTime)
{
}

void PlayerMovementComponent::OnEvent(EventType type, const void* data)
{
	// 턴 변경: 플레이어 턴 아닐 때는 Cancel 이벤트만 발행
	if (type == EventType::TurnChanged)
	{
		const auto* payload = static_cast<const Events::TurnChanged*>(data);
		if (!payload) return;

		if (static_cast<Turn>(payload->turn) != Turn::PlayerTurn)
		{
			auto* owner = GetOwner();
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Cancel");

			// 입력 상태 정리(프리뷰/원복은 FSM 액션에서 처리)
			m_HasDragRay = false;
			m_DragStartNode = nullptr;
		}
		return;
	}

	const auto* mouseData = static_cast<const Events::MouseState*>(data);
	if (!mouseData) return;
	if (mouseData->handled) return;

	auto* owner = GetOwner();
	if (!owner) return;

	auto* scene = owner->GetScene();
	if (!scene) return;

	if (!scene->GetServices().Has<InputManager>())
		return;

	auto& input = scene->GetServices().Get<InputManager>();
	auto camera = scene->GetGameCamera();
	if (!camera) return;

	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!transComp) return;

	auto* player = owner->GetComponent<PlayerComponent>();
	if (!player) return;

	auto* moveFsm = owner->GetComponent<PlayerMoveFSMComponent>();
	if (!moveFsm) return;

	if (type == EventType::KeyDown)
	{
		auto* player = owner->GetComponent<PlayerComponent>();


		// Turn Check.
		if(!player || player->GetCurrentTurn() != Turn::PlayerTurn)
		{
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Cancel");
			m_HasDragRay = false;
			return;
		}

		auto keyData = static_cast<const Events::KeyEvent*>(data);
		if (!keyData) return;

		if(keyData->key == VK_ESCAPE)
			DispatchMoveEvent(owner, "Move_Revoke");
	}

	if (type == EventType::MouseRightClick)
	{
		// 턴 아니면 cancel
		if (player->GetCurrentTurn() != Turn::PlayerTurn)
		{
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Cancel");
			m_HasDragRay = false;
			return;
		}

		DispatchMoveEvent(owner, "Move_Revoke");
	}

	// MouseUp: Commit/Cancel 의사만 FSM에 전달
	if (type == EventType::MouseLeftClickUp)
	{
		// 턴 아니면 cancel
		if (player->GetCurrentTurn() != Turn::PlayerTurn)
		{
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Cancel");
			m_HasDragRay = false;
			return;
		}

		if (!moveFsm->HasPendingTarget())
		{
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Cancel");
			m_HasDragRay = false;
			return;
		}


		// 드래그(Selecting) 중일 때만 Confirm 올리는게 정상
		if (moveFsm->HasPendingTarget())
		{
			DispatchMoveEvent(owner, "Move_Confirm");
			const int targetQ = moveFsm->GetPeningQ();
			const int targetR = moveFsm->GetPeningR();
			const bool consumed = player->CommitMove(targetQ, targetR);
			if (!consumed)
			{
				transComp->SetPosition(m_DragStartPos);
			}
			else
			{
				ApplyRotationForMove(targetQ, targetR);
			}
		}
		else
		{
			DispatchMoveEvent(owner, "Move_Revoke");
		}

		m_HasDragRay = false;
		return;
	}

	if (!input.IsPointInViewport(mouseData->pos))
		return;

	// MouseDown: Select 의사만 FSM에 전달
	if (type == EventType::MouseLeftClick)
	{
		if (player->GetCurrentTurn() != Turn::PlayerTurn)
			return;

		Ray pickRay{};
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
			return;

		// 선택 판정(본인 or 노드 or 가장 가까운 collider)
		auto* collider = owner->GetComponent<BoxColliderComponent>();
		float ownerHitT = 0.0f;
		const bool hitOwner = collider && collider->HasBounds()
			&& collider->IntersectsRay(pickRay.m_Pos, pickRay.m_Dir, ownerHitT);

		const auto pos = transComp->GetPosition();
		float nodeHitT = 0.0f;
		NodeComponent* clickedNode = FindClosestNodeHit(scene, pickRay.m_Pos, pickRay.m_Dir, nodeHitT);

		auto& gameObjects = scene->GetGameObjects();
		float closestT = FLT_MAX;
		GameObject* closestObject = nullptr;

		for (const auto& [name, object] : gameObjects)
		{
			if (!object) continue;

			auto* otherCollider = object->GetComponent<BoxColliderComponent>();
			if (!otherCollider || !otherCollider->HasBounds())
				continue;

			float hitT = 0.0f;
			if (!otherCollider->IntersectsRay(pickRay.m_Pos, pickRay.m_Dir, hitT))
				continue;

			if (hitT >= 0.0f && hitT < closestT)
			{
				closestT = hitT;
				closestObject = object.get();
			}
		}

		if (!hitOwner && closestObject != owner)
			return;

		// 드래그 오프셋 계산(입력 기반 데이터)
		if (clickedNode)
		{
			auto* nodeOwner = clickedNode->GetOwner();
			auto* nodeTransform = nodeOwner ? nodeOwner->GetComponent<TransformComponent>() : nullptr;
			if (nodeTransform)
			{
				const auto nodePos = nodeTransform->GetPosition();
				m_DragOffset = { 0.0f, pos.y - nodePos.y, 0.0f };
			}
			else
			{
				m_DragOffset = { 0.0f, 0.0f, 0.0f };
			}
		}
		else
		{
			m_DragOffset = { 0.0f, 0.0f, 0.0f };
		}

		// 레이/시작 위치 저장
		m_DragRayOrigin = pickRay.m_Pos;
		m_DragRayDir = pickRay.m_Dir;
		m_HasDragRay = true;

		m_DragStartPos = transComp->GetPosition();
		m_DragStartNode = nullptr;

		if (m_GridSystem)
			m_DragStartNode = m_GridSystem->GetNodeByKey({ player->GetQ(), player->GetR() });

		// FSM에 상태 시작 의사 전달 (BeginMove/드래그 활성화는 FSM 액션에서)
		DispatchPlayerStateEvent(owner, "Move_Start");
		DispatchMoveEvent(owner, "Move_Select");
		return;
	}

	// Dragged: 레이만 갱신
	if (type != EventType::Dragged)
		return;

	// 드래그 활성은 FSM이 관리하지만, 레이는 계속 갱신해줘야 프리뷰가 움직임
	if (!moveFsm->IsDraggingActive())
		return;

	Ray dragRay{};
	if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, dragRay))
		return;

	m_DragRayOrigin = dragRay.m_Pos;
	m_DragRayDir = dragRay.m_Dir;
	m_HasDragRay = true;
}

void PlayerMovementComponent::ApplyRotationForMove(int targetQ, int targetR)
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!transComp || !m_GridSystem)
		return;

	auto* player = owner->GetComponent<PlayerComponent>();
	if (!player)
		return;

	const AxialKey startKey = m_DragStartNode
		? AxialKey{ m_DragStartNode->GetQ(), m_DragStartNode->GetR() }
	: AxialKey{ player->GetQ(), player->GetR() };
	const AxialKey targetKey{ targetQ, targetR };
	const auto path = m_GridSystem->GetShortestPath(startKey, targetKey);

	// 경로가 2개 이상일 때 마지막 스텝 방향을 사용한다. (직전 노드 -> 목적지)
	if (path.size() < 2)
		return;

	const AxialKey& previousKey = path[path.size() - 2];
	const AxialKey& currentKey = path.back();
	RotationOffset rotation{};
	if (TryGetRotationFromStep(previousKey, currentKey, rotation))
	{
		SetPlayerRotation(transComp, rotation);
	}
}

bool PlayerMovementComponent::IsDragging() const
{
	auto* owner = GetOwner();
	auto* moveFSM = owner ? owner->GetComponent<PlayerMoveFSMComponent>() : nullptr;
	return moveFSM && moveFSM->IsDraggingActive();
}



void PlayerMovementComponent::SetPlayerRotation(TransformComponent* transComp, RotationOffset dir)
{
	if (!transComp){return;}

	switch (dir)
	{
	case RotationOffset::clock_1:
		transComp->SetRotationEuler({ 0.0f,-150.0f ,0.0f });
		break;
	case RotationOffset::clock_3:
		transComp->SetRotationEuler({ 0.0f,-90.0f ,0.0f });
		break;
	case RotationOffset::clock_5:
		transComp->SetRotationEuler({ 0.0f,-30.0f ,0.0f });
		break;
	case RotationOffset::clock_7:
		transComp->SetRotationEuler({ 0.0f,30.0f ,0.0f });
		break;
	case RotationOffset::clock_9:
		transComp->SetRotationEuler({ 0.0f,90.0f ,0.0f });
		break;
	case RotationOffset::clock_11:
		transComp->SetRotationEuler({ 0.0f,150.0f ,0.0f });
		break;
	default:
		break;
	}
}
