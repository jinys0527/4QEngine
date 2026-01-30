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

PlayerMovementComponent::PlayerMovementComponent()
{
}

PlayerMovementComponent::~PlayerMovementComponent()
{
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().RemoveListener(EventType::Dragged, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClickUp, this);
}

void PlayerMovementComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::KeyDown, this); // 이벤트 추가
	GetEventDispatcher().AddListener(EventType::KeyUp, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().AddListener(EventType::Dragged, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClickUp, this);
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

	if (!m_IsDragging || !m_HasDragRay)
		return;

	auto* owner = GetOwner(); //
	if (!owner)
		return;

	auto* scene = owner->GetScene();
	if (!scene) return;

	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!transComp)
		return;

	float nodeHitT = 0.0f;
	NodeComponent* targetNode = FindClosestNodeHit(scene, m_DragRayOrigin, m_DragRayDir, nodeHitT);
	if (!targetNode)
	{
		m_CurrentTargetNode = nullptr;
		return;
	}

	if (!targetNode->GetIsMoveable())
	{
		m_CurrentTargetNode = nullptr;
		return;
	}

	if (!targetNode->IsInMoveRange())
	{
		m_CurrentTargetNode = nullptr;
		return;
	}

	const auto state = targetNode->GetState();
	if (state != NodeState::Empty && state != NodeState::HasPlayer)
	{
		m_CurrentTargetNode = nullptr;
		return;
	}

	auto* targetOwner = targetNode->GetOwner();
	auto* targetTransform = targetOwner ? targetOwner->GetComponent<TransformComponent>() : nullptr;
	if (!targetTransform)
	{
		m_CurrentTargetNode = nullptr;
		return;
	}

	const auto nodePos = targetTransform->GetPosition();
	DirectX::XMFLOAT3 newPos{
		nodePos.x + m_DragOffset.x,
		nodePos.y + m_DragOffset.y,
		nodePos.z + m_DragOffset.z
	};

	transComp->SetPosition(newPos);
	m_CurrentTargetNode = targetNode;
}

void PlayerMovementComponent::OnEvent(EventType type, const void* data)
{
	const auto* mouseData = static_cast<const Events::MouseState*>(data);
	if (!mouseData)
		return;

	if (mouseData->handled)
		return;

	auto* owner = GetOwner();
	if (!owner)
		return;

	auto* scene = owner->GetScene();
	if (!scene)
		return;

	if (!scene->GetServices().Has<InputManager>())
		return;

	auto& input = scene->GetServices().Get<InputManager>();
	auto camera = scene->GetGameCamera();
	if (!camera)
		return;

	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!transComp)
		return;

	if (type == EventType::MouseLeftClickUp)
	{
		auto* player = owner->GetComponent<PlayerComponent>();


		// Turn Check.
		if(!player || player->GetCurrentTurn() != Turn::PlayerTurn)
		{
			if (m_IsDragging)
			{
				transComp->SetPosition(m_DragStartPos);
				m_IsDragging = false;
				m_HasDragRay = false;
				m_CurrentTargetNode = nullptr;
			}
			return;
		}

		if (m_CurrentTargetNode && player)
		{
			const bool consumed = player->CommitMove(m_CurrentTargetNode->GetQ(), m_CurrentTargetNode->GetR());
			if (!consumed)
			{
				transComp->SetPosition(m_DragStartPos);
			}
			else if (m_GridSystem)
			{
				// 커밋 직전: 실제 최단 경로를 구해 마지막 두 노드로 회전 방향을 결정한다.
				const AxialKey startKey = m_DragStartNode
					? AxialKey{ m_DragStartNode->GetQ(), m_DragStartNode->GetR() }
				: AxialKey{ player->GetQ(), player->GetR() };
				const AxialKey targetKey{ m_CurrentTargetNode->GetQ(), m_CurrentTargetNode->GetR() };
				const auto path = m_GridSystem->GetShortestPath(startKey, targetKey);

				// 경로가 2개 이상일 때 마지막 스텝 방향을 사용한다. (직전 노드 -> 목적지)
				if (path.size() >= 2)
				{
					const AxialKey& previousKey = path[path.size() - 2];
					const AxialKey& currentKey = path.back();
					RotationOffset rotation{};
					if (TryGetRotationFromStep(previousKey, currentKey, rotation))
					{
						SetPlayerRotation(transComp, rotation);
					}
				}
			}
		}

		m_IsDragging = false;
		m_HasDragRay = false;
		m_CurrentTargetNode = nullptr;
		m_DragStartNode = nullptr;
		return;
	}

	if (!input.IsPointInViewport(mouseData->pos))
		return;

	// Logic
	if (type == EventType::MouseLeftClick)
	{
		auto* player = owner->GetComponent<PlayerComponent>();
		if (!player || player->GetCurrentTurn() != Turn::PlayerTurn) 
			return;
	
		Ray pickRay{};
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
			return;

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

		// 가장 가까운 collider hit 찾기 (선택 판정)
		for (const auto& [name, object] : gameObjects)
		{
			if (!object)
				continue;

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

		// 2) 드래그 기준 표면 hit (바닥/발판). 자기 자신은 제외.

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

		m_DragRayOrigin = pickRay.m_Pos;
		m_DragRayDir    = pickRay.m_Dir;
		m_IsDragging    = true;
		m_HasDragRay    = true;

		m_DragStartPos = transComp->GetPosition();
		m_CurrentTargetNode = nullptr;
		if (auto* player = owner->GetComponent<PlayerComponent>())
		{
			player->BeginMove();
			if (m_GridSystem)
			{
				m_DragStartNode = m_GridSystem->GetNodeByKey({ player->GetQ(), player->GetR() });
			}
		}

		return;
	}

	// 드래그 중: 레이만 갱신
	if (type != EventType::Dragged || !m_IsDragging)
		return;

	Ray dragRay{};
	if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, dragRay))
		return;

	m_DragRayOrigin = dragRay.m_Pos;
	m_DragRayDir    = dragRay.m_Dir;
	m_HasDragRay    = true;
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
