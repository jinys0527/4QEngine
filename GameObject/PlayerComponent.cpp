#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "CameraObject.h"
#include "Scene.h"
#include "GridSystemComponent.h"
#include "ServiceRegistry.h"
#include "ItemComponent.h"
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"
#include "Event.h"
#include "InputManager.h"
#include "RayHelper.h"
#include "PlayerStatComponent.h"
#include "SkeletalMeshComponent.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "NodeComponent.h"
#include "PushNodeComponent.h"
#include <algorithm>
#include <cfloat>
#include "SkinningAnimationComponent.h"
#include "MathHelper.h"
#include <cmath>
#include "GameManager.h"
#include "CombatManager.h"
#include "DoorComponent.h"
#include "PlayerCombatFSMComponent.h"
#include "PlayerFSMComponent.h"
#include "PlayerDoorFSMComponent.h"
#include "DiceSystem.h"

REGISTER_COMPONENT(PlayerComponent)
REGISTER_PROPERTY_READONLY(PlayerComponent, Q)
REGISTER_PROPERTY_READONLY(PlayerComponent, R)
REGISTER_PROPERTY(PlayerComponent, MoveResource)
REGISTER_PROPERTY_READONLY(PlayerComponent, RemainMoveResource)
REGISTER_PROPERTY(PlayerComponent, ActResource)
REGISTER_PROPERTY_READONLY(PlayerComponent, RemainActResource)
REGISTER_PROPERTY(PlayerComponent, CurrentWeaponCost)
REGISTER_PROPERTY(PlayerComponent, AttackRange)
REGISTER_PROPERTY(PlayerComponent, Money)
REGISTER_PROPERTY(PlayerComponent, DebugEquipItem)


//REGISTER_PROPERTY(PlayerComponent, Item)

static int AxialDistance(int q1, int r1, int q2, int r2)
{
	const int dq = q1 - q2;
	const int dr = r1 - r2;
	const int ds = dq + dr;
	return (std::abs(dq) + std::abs(dr) + std::abs(ds)) / 2;
}

namespace
{
	float DistanceSq2D(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;
		return dx * dx + dz * dz;
	}

	GameObject* FindGameObjectByName(Scene* scene, const std::string& name)
	{
		if (!scene || name.empty())
		{
			return nullptr;
		}

		const auto& objects = scene->GetGameObjects();
		auto it = objects.find(name);
		if (it == objects.end())
		{
			return nullptr;
		}

		return it->second.get();
	}


	NodeComponent* FindClosestNodeByPosition(GridSystemComponent* grid, const XMFLOAT3& position)
	{
		if (!grid)
		{
			return nullptr;
		}

		float closestDistSq = FLT_MAX;
		NodeComponent* closestNode = nullptr;
		for (auto* node : grid->GetNodes())
		{
			if (!node)
			{
				continue;
			}

			auto* nodeOwner = node->GetOwner();
			auto* nodeTransform = nodeOwner ? nodeOwner->GetComponent<TransformComponent>() : nullptr;
			if (!nodeTransform)
			{
				continue;
			}

			const float distSq = DistanceSq2D(nodeTransform->GetPosition(), position);
			if (distSq < closestDistSq)
			{
				closestDistSq = distSq;
				closestNode = node;
			}
		}

		return closestNode;
	}

	ItemComponent* FindClosestItemHit(Scene* scene, const Ray& ray, float& outT)
	{
		if (!scene)
		{
			return nullptr;
		}

		float closestT = FLT_MAX;
		ItemComponent* closestItem = nullptr;

		for (const auto& [name, object] : scene->GetGameObjects())
		{
			(void)name;
			if (!object)
			{
				continue;
			}

			auto* item = object->GetComponent<ItemComponent>();
			if (!item)
			{
				continue;
			}

			auto* collider = object->GetComponent<BoxColliderComponent>();
			if (!collider || !collider->HasBounds())
			{
				continue;
			}

			float hitT = 0.0f;
			if (!collider->IntersectsRay(ray.m_Pos, ray.m_Dir, hitT))
			{
				continue;
			}

			if (hitT >= 0.0f && hitT < closestT)
			{
				closestT = hitT;
				closestItem = item;
			}
		}

		if (!closestItem)
		{
			return nullptr;
		}

		outT = closestT;
		return closestItem;
	}

	void DispatchPlayerStateEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerFSMComponent>())
			fsm->DispatchEvent(eventName);
	}

	void DispatchCombatEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerCombatFSMComponent>())
			fsm->DispatchEvent(eventName);
	}

	void DispatchDoorEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerDoorFSMComponent>())
			fsm->DispatchEvent(eventName);
	}
}

static NodeComponent* FindClosestNodeHit(Scene* scene, const Ray& ray, float& outT)
{
	if (!scene)
	{
		return nullptr;
	}

	float closestT = FLT_MAX;
	NodeComponent* closestNode = nullptr;

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		if (!object)
		{
			continue;
		}

		auto* node = object->GetComponent<NodeComponent>();
		if (!node)
		{
			continue;
		}

		auto* collider = object->GetComponent<BoxColliderComponent>();
		if (!collider || !collider->HasBounds())
		{
			continue;
		}

		float hitT = 0.0f;
		if (!collider->IntersectsRay(ray.m_Pos, ray.m_Dir, hitT))
		{
			continue;
		}

		if (hitT >= 0.0f && hitT < closestT)
		{
			closestT = hitT;
			closestNode = node;
		}
	}

	if (!closestNode)
	{
		return nullptr;
	}

	outT = closestT;
	return closestNode;
}

static EnemyComponent* FindEnemyInRange(GridSystemComponent* grid, int playerQ, int playerR, int range)
{
	if (!grid || range < 0)
	{
		return nullptr;
	}

	for (auto* enemy : grid->GetEnemies())
	{
		if (!enemy || enemy->GetActorId() == 0)
		{
			continue;
		}

		const int distance = AxialDistance(playerQ, playerR, enemy->GetQ(), enemy->GetR());
		if (distance <= range)
		{
			return enemy;
		}
	}

	return nullptr;
}

static EnemyComponent* FindEnemyAt(GridSystemComponent* grid, int q, int r)
{
	if (!grid)
	{
		return nullptr;
	}



	for (auto* enemy : grid->GetEnemies())
	{
		if (!enemy)
		{
			continue;
		}
		auto* enemyOwner = enemy->GetOwner();
		auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
		if (enemyStat && enemyStat->IsDead())
		{
			continue;
		}
		if (enemy->GetQ() == q && enemy->GetR() == r)
		{
			return enemy;
		}
	}

	return nullptr;
}



PlayerComponent::PlayerComponent() {

}

PlayerComponent::~PlayerComponent() {
	// Event Listener 쓰는 경우만	
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseRightClick, this);
}

void PlayerComponent::Start()
{
	//start시 GameManagaer get
	ResetTurnResources();
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene) { return; }

	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().AddListener(EventType::MouseRightClick, this);
	const auto& objects = scene->GetGameObjects();

	for (const auto& [name,object] : objects) {
		if (!object) { continue; }
		
		if (auto* grid = object->GetComponent<GridSystemComponent>()) {
			m_GridSystem = grid;
			break;
		}
	}

	m_DebugEquipItem = false;

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

	auto* gameManager = scene->GetGameManager();
	const bool allowExplorationTurn = !gameManager && m_CurrentTurn == Turn::PlayerTurn;
	//아이템 장착 테스트


	//Player Turn 종료 조건
// 	if (allowExplorationTurn) {
// 		m_TurnElapsed += deltaTime;
// 
// 		if (!m_TurnEndRequested && m_TurnElapsed >= m_PlayerTurnTime) {
// 			//종료(턴 전환)
// 			GetEventDispatcher().Dispatch(EventType::PlayerTurnEndRequested, nullptr);
// 			m_TurnEndRequested = true;
// 		}
// 	}
	// 이제 전체 턴 관리하는 GameManager에서 넘김 여기선 UI에서 턴 종료했을때만 처리하면 될듯


	//임시로 첫번째 자식을 가지고 있는 아이템으로 지정
	//auto* transformcomponent = owner->GetComponent<TransformComponent>();
	//{
	//	if (!transformcomponent->GetChildrens().empty() && m_MeeleItem == nullptr)
	//	{
	//		GameObject* item = dynamic_cast<GameObject*>(transformcomponent->GetChildrens()[0]->GetOwner());
	//		auto* itemcomp = item->GetComponent<ItemComponent>();
	//		if (itemcomp && itemcomp->GetType() == 1)
	//		{
	//			m_MeeleItem = item;
	//			itemcomp->SetIsEquiped(true);
	//			m_InventoryItemIds.push_back(item->GetName());

	//		}
	//	}

	//}

	//근접 무기 스탯 적용
	if (m_MeeleItem != nullptr)
	{
		auto* itemcomponent = m_MeeleItem->GetComponent<ItemComponent>();
		if (!itemcomponent) return;

		//근접 무기의 스탯 적용하기
		if (!m_IsApplyMeeleStat)
		{
			auto* playerstatcomponent = owner->GetComponent<PlayerStatComponent>();
			if (!playerstatcomponent) return;

			int health = playerstatcomponent->GetHealth();
			int strength = playerstatcomponent->GetStrength();
			int agility = playerstatcomponent->GetAgility();
			int sense = playerstatcomponent->GetSense();
			int skill = playerstatcomponent->GetSkill();

			int ihealth = itemcomponent->GetHealth();
			int istrength = itemcomponent->GetStrength();
			int iagility = itemcomponent->GetAgility();
			int isense = itemcomponent->GetSense();
			int iskill = itemcomponent->GetSkill();
			int idefense = itemcomponent->GetDEF();
			int irange = itemcomponent->GetMeleeAttackRange();

			playerstatcomponent->SetHealth(health + ihealth);
			playerstatcomponent->SetStrength(strength + istrength);
			playerstatcomponent->SetAgility(agility + iagility);
			playerstatcomponent->SetSense(sense + isense);
			playerstatcomponent->SetSkill(skill + iskill);
			playerstatcomponent->SetEquipmentDefenseBonus(idefense);
			playerstatcomponent->SetRange(irange);

			m_IsApplyMeeleStat = true;
		}
	}
	
	//근접 무기 모드면 근접무기 들기
	if (/*m_IsMeleeMode && */m_MeeleItem != nullptr)
	{
		auto* itemcomponent = m_MeeleItem->GetComponent<ItemComponent>();
		if (!itemcomponent) return;


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

		XMFLOAT4X4 equipmentPose = skeleton->equipmentBindPose;
		const int equipmentBoneIndex = skeleton->equipmentBoneIndex;
		if (equipmentBoneIndex >= 0)
		{
			const auto* animComp = owner->GetComponent<SkinningAnimationComponent>();
			if (animComp)
			{
				const auto& globalPose = animComp->GetGlobalPose();
				if (static_cast<size_t>(equipmentBoneIndex) < globalPose.size())
				{
					equipmentPose = globalPose[static_cast<size_t>(equipmentBoneIndex)];
				}
			}
		}


		// equipment 본 포즈 로드
		XMMATRIX equipmentM = XMLoadFloat4x4(&equipmentPose);

		// 스케일 적용 (회전 보존)
		XMMATRIX scaleM = XMMatrixScaling(0.01f, 0.01f, 0.01f);
		equipmentM = XMMatrixMultiply(scaleM, equipmentM);

		// 플레이어 월드 행렬
		auto* playerTransform = owner->GetComponent<TransformComponent>();
		if (!playerTransform) return;

		XMMATRIX playerWorldM = XMLoadFloat4x4(&playerTransform->GetWorldMatrix());

		XMMATRIX finalM = XMMatrixMultiply(equipmentM, playerWorldM);

		XMFLOAT4X4 finalPose;
		XMStoreFloat4x4(&finalPose, finalM);

		// 최종 적용
		itemcomponent->SetEquipmentBindPose(finalPose);
	}
}

void PlayerComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::MouseLeftClick)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;

		// 전투 Input
		if (gameManager && gameManager->IsCombatInputAllowed())
		{
			if (!scene || !scene->GetServices().Has<InputManager>())
			{
				return;
			}

			auto& input = scene->GetServices().Get<InputManager>();
			if (!input.IsPointInViewport(mouseData->pos))
			{
				return;
			}

			auto camera = scene->GetGameCamera();
			if (!camera)
			{
				return;
			}

			Ray pickRay{};
			if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
			{
				return;
			}

			float hitT = 0.0f;
			auto* clickedNode = FindClosestNodeHit(scene, pickRay, hitT);
			if (!clickedNode)
			{
				return;
			}

			auto* enemy = FindEnemyAt(m_GridSystem, clickedNode->GetQ(), clickedNode->GetR());
			if (!enemy)
			{
				return;
			}

			const int range = max(0, m_AttackRange);
			const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
			if (distance > range)
			{
				return;
			}

			if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
			{	
				m_PendingAttackTarget = enemy;
				if (combatFsm->TryExecutePlayerAttackFromInput())
				{
					//return;
					mouseData->handled = true;
				}
			}
			return;
		}

		// 이동 Input

		if (!gameManager || !gameManager->IsExplorationInputAllowed())
		{
			return;
		}

		if (!scene || !scene->GetServices().Has<InputManager>())
		{
			return;
		}

		auto& input = scene->GetServices().Get<InputManager>();
		if (!input.IsPointInViewport(mouseData->pos))
		{
			return;
		}

		auto camera = scene->GetGameCamera();
		if (!camera)
		{
			return;
		}

		Ray pickRay{};
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
		{
			return;
		}

		//아이템 줍기
		float hitT = 0.0f;
		if (auto* clickedItem = FindClosestItemHit(scene, pickRay, hitT))
		{
			if (TryPickup(clickedItem))
			{
				mouseData->handled = true;
				return;
			}
		}
		hitT = 0.0f;


		BeginThrowPreview();




		auto* clickedNode = FindClosestNodeHit(scene, pickRay, hitT);
		if (!clickedNode)
		{
			return;
		}
		
		std::cout << clickedNode->GetQ() << ", " << clickedNode->GetR() << std::endl; // 클릭된 Node Debug

		if (!clickedNode->GetIsMoveable())
		{

			if (auto* door = clickedNode->GetLinkedDoor())
			{
				//std::cout << "Door Evenet Detected" << std::endl;
				// 인접 노드 판별
				const int distanceToDoor = AxialDistance(m_Q, m_R, clickedNode->GetQ(), clickedNode->GetR());
				if (distanceToDoor > 1)
				{
					return;
				}
				m_PendingDoor = door;
				DispatchPlayerStateEvent(owner, "Door_Interact");
				
				mouseData->handled = true;
				return;
			}
		}

		auto* enemy = FindEnemyAt(m_GridSystem, clickedNode->GetQ(), clickedNode->GetR());
		if (!enemy)
		{
			return;
		}

		const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
		if (m_IsThrowPreviewActive)
		{
			int throwRange = 0;
			ItemComponent* throwItem = nullptr;
			if (TryGetConsumableThrowRange(throwRange) && TryGetConsumableThrowItem(throwItem))
			{
				if (distance <= throwRange && ApplyThrowDamage(throwItem, enemy))
				{
					auto* playerTransform = owner ? owner->GetComponent<TransformComponent>() : nullptr;
					auto* enemyOwner = enemy->GetOwner();
					auto* enemyTransform = enemyOwner ? enemyOwner->GetComponent<TransformComponent>() : nullptr;

					XMFLOAT3 startPos = playerTransform ? playerTransform->GetPosition() : XMFLOAT3{};
					XMFLOAT3 targetPos = enemyTransform ? enemyTransform->GetPosition() : XMFLOAT3{};

					// y값을 1.0f 위로 보정
					startPos.y += 1.0f;
					targetPos.y += 1.0f;

					throwItem->BeginThrow(startPos, targetPos, 2.0f);
					ConsumeThrowItem(throwItem);
					mouseData->handled = true;
					return;
				}
			}
		}

		const int range = max(0, m_AttackRange);
		if (distance > range)
		{
			return;
		}

		if (!m_IsMeleeMode)
		{
			std::cout << "MeleeMode\n";
			m_IsMeleeMode = true;
		}
		else
		{
			if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
			{
				m_CombatConfirmRequested = true;
				combatFsm->RequestCombatEnter(GetActorId(), enemy->GetActorId());
			}
		}
		

		return;
	}

	if (type == EventType::MouseLeftDoubleClick)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;

		if (gameManager && gameManager->IsCombatInputAllowed())
		{
			if (!scene || !scene->GetServices().Has<InputManager>())
			{
				return;
			}

			auto& input = scene->GetServices().Get<InputManager>();
			if (!input.IsPointInViewport(mouseData->pos))
			{
				return;
			}

			auto camera = scene->GetGameCamera();
			if (!camera)
			{
				return;
			}

			Ray pickRay{};
			if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
			{
				return;
			}

			float hitT = 0.0f;
			auto* clickedNode = FindClosestNodeHit(scene, pickRay, hitT);
			if (!clickedNode)
			{
				return;
			}

			auto* enemy = FindEnemyAt(m_GridSystem, clickedNode->GetQ(), clickedNode->GetR());
			if (!enemy)
			{
				return;
			}

			const int range = max(0, m_AttackRange);
			const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
			if (distance > range)
			{
				return;
			}

			if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
			{
				m_PendingAttackTarget = enemy;
				if (combatFsm->TryExecutePlayerAttackFromInput())
				{
					//return;
					mouseData->handled = true;
				}
			}
			return;
		}

		if (!gameManager || !gameManager->IsExplorationInputAllowed())
		{
			return;
		}

		if (!scene || !scene->GetServices().Has<InputManager>())
		{
			return;
		}

		auto& input = scene->GetServices().Get<InputManager>();
		if (!input.IsPointInViewport(mouseData->pos))
		{
			return;
		}

		auto camera = scene->GetGameCamera();
		if (!camera)
		{
			return;
		}

		Ray pickRay{};
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
		{
			return;
		}

		float hitT = 0.0f;
		auto* clickedNode = FindClosestNodeHit(scene, pickRay, hitT);
		if (!clickedNode)
		{
			return;
		}

		auto* enemy = FindEnemyAt(m_GridSystem, clickedNode->GetQ(), clickedNode->GetR());
		if (!enemy)
		{
			return;
		}

		const int range = max(0, m_AttackRange);
		const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
		if (distance > range)
		{
			return;
		}

		if (!m_IsMeleeMode)
		{
			std::cout << "MeleeMode\n";
			m_IsMeleeMode = true;
		}

		if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
		{
			m_CombatConfirmRequested = true;
			combatFsm->RequestCombatEnter(GetActorId(), enemy->GetActorId());
		}

		return;
	}

	if (type == EventType::MouseRightClick)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;

		if(gameManager->GetCombatManager()->GetState() != Battle::InBattle)
		{
			std::cout << "IdleMode\n";

			m_IsMeleeMode = false;
		}
	}

	if (type == EventType::MouseRightClickHold)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;
		if (gameManager && !gameManager->IsExplorationInputAllowed())
		{
			return;
		}

		
		return;
	}

	if (type == EventType::MouseRightClickUp)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		EndThrowPreview();
		return;
	}

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
	m_TurnEndRequested = false;
	if (m_CurrentTurn == Turn::PlayerTurn)
	{
		ResetTurnResources();
	}
	else
	{
		EndThrowPreview();
	}
}

// 행동,이동력 초기화 // turn 초기화
void PlayerComponent::ResetTurnResources()
{
	m_RemainMoveResource = m_MoveResource;
	m_RemainActResource = m_ActResource;
	m_HasMoveStart = false;
	m_CombatConfirmRequested = false;
	m_SelectedEnemy = nullptr;
	EndThrowPreview();
	ResetSubFSMFlags();
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
	if (m_CurrentTurn != Turn::PlayerTurn)
	{
		return false;
	}

	const int startQ = m_HasMoveStart ? m_StartQ : m_Q;
	const int startR = m_HasMoveStart ? m_StartR : m_R;
	//
	int cost = -1;
	if (m_GridSystem) {
		cost = m_GridSystem->GetShortestPathLength({ startQ,startR }, { targetQ,targetR });
	}
	if (cost < 0) {
		cost = AxialDistance(startQ, startR, targetQ, targetR); // start와 Target 간의 소모 cost return
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
	if (m_CurrentTurn != Turn::PlayerTurn)
	{
		return false;
	}
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

void PlayerComponent::RequestCombatConfirm()
{
	m_CombatConfirmRequested = true;
}

bool PlayerComponent::HandleCombatClick(EnemyComponent* enemy)
{
	if (!enemy)
	{
		ClearCombatSelection();
		return false;
	}

	if (m_CurrentTurn != Turn::PlayerTurn)
	{
		return false;
	}

	const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
	if (distance > 1)
	{
		return false;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	if (m_SelectedEnemy == enemy)
	{
		m_PendingAttackTarget = enemy;
		RequestCombatConfirm();
		DispatchCombatEvent(owner, "Combat_Confirm");
		m_SelectedEnemy = nullptr;
		return true;
	}

	m_SelectedEnemy = enemy;
	m_PendingAttackTarget = enemy;
	DispatchPlayerStateEvent(owner, "Combat_Start");
	return true;
}

void PlayerComponent::ClearCombatSelection()
{
	m_SelectedEnemy = nullptr;
	m_PendingAttackTarget = nullptr;
}

EnemyComponent* PlayerComponent::ResolveCombatTarget(GameObject* obj) const
{
	if (!obj) return nullptr;
	if (auto* enemy = obj->GetComponent<EnemyComponent>()) return enemy;
	if (auto* node = obj->GetComponent<NodeComponent>())
	{
		if (m_GridSystem)
			return m_GridSystem->GetEnemyAt(node->GetQ(), node->GetR());
	}
	return nullptr;
}

EnemyComponent* PlayerComponent::ConsumePendingAttackTarget()
{
	EnemyComponent* target = m_PendingAttackTarget;
	m_PendingAttackTarget = nullptr;
	return target;
}


// 밀기 관련

// 조건 체크
bool PlayerComponent::TryFindPushTarget(EnemyComponent*& outEnemy, NodeComponent*& outNode) const
{
	outEnemy = nullptr;
	outNode = nullptr;
	if (!m_GridSystem)
	{
		return false;
	}

	const int playerQ = m_Q;
	const int playerR = m_R;
	const auto& enemies = m_GridSystem->GetEnemies();

	for (auto* enemy : enemies) {
		if (!enemy) { continue; }

		const int distance = AxialDistance(playerQ, playerR, enemy->GetQ(), enemy->GetR());

		if (distance != 1) { continue; }

		auto* enemyOwner = enemy->GetOwner();

		if (!enemyOwner) { continue; }

		if (auto* enemyStat = enemyOwner->GetComponent<EnemyStatComponent>())
		{
			if (enemyStat->IsDead()) { continue; }
		}

		const int dq = enemy->GetQ() - playerQ;
		const int dr = enemy->GetR() - playerR;
		const AxialKey targetKey{ enemy->GetQ() + dq ,enemy->GetR() + dr };
		auto* targetNode = m_GridSystem->GetNodeByKey(targetKey);

		if (!targetNode) { continue; }

		auto* targetOwner = targetNode->GetOwner();
		if (!targetOwner || !targetOwner->GetComponent<PushNodeComponent>()) { continue; }
		// if (targetNode->GetState() != NodeState::Empty) { continue; } 
		outEnemy = enemy;
		outNode = targetNode;
		return true; // 밀기 세팅 완료
	}

	return false;
}

// 밀기 동작
bool PlayerComponent::ResolvePushTarget(EnemyComponent* enemy, NodeComponent* targetNode)
{
	if (!enemy || !targetNode || !m_GridSystem){ return false;}

	auto* enemyOwner = enemy->GetOwner();
	if (!enemyOwner){return false;}

	auto* enemyTransform = enemyOwner->GetComponent<TransformComponent>();
	auto* targetOwner = targetNode->GetOwner();
	auto* targetTransform = targetOwner ? targetOwner->GetComponent<TransformComponent>() : nullptr;


	if (!enemyTransform || !targetTransform){return false;}

	enemyTransform->SetPosition(targetTransform->GetPosition());
	enemy->SetQR(targetNode->GetQ(), targetNode->GetR());

	if (auto* enemyStat = enemyOwner->GetComponent<EnemyStatComponent>())
	{
		enemyStat->SetCurrentHP(0);
	}

	return true;
}

void PlayerComponent::ClearPendingPush()
{
	m_PendingPushEnemy = nullptr;
	m_PendingPushNode = nullptr;
}

bool PlayerComponent::ConsumeCombatConfirmRequest()
{
	if (!m_CombatConfirmRequested)
	{
		return false;
	}

	m_CombatConfirmRequested = false;
	return true;
}

bool PlayerComponent::ConsumePushPossible()
{
	//return ConsumeFlag(m_PushPossible);
	EnemyComponent* enemy = nullptr;
	NodeComponent* targetNode = nullptr;
	const bool possible = TryFindPushTarget(enemy, targetNode);
	if (possible)
	{
		m_PendingPushEnemy = enemy;
		m_PendingPushNode = targetNode;
	}
	else
	{
		ClearPendingPush();
	}
	return possible;
}


bool PlayerComponent::ConsumePushTargetFound()
{
	//return ConsumeFlag(m_PushTargetFound);
	if (m_PendingPushEnemy && m_PendingPushNode)
	{
		return true;
	}

	EnemyComponent* enemy = nullptr;
	NodeComponent* targetNode = nullptr;
	const bool found = TryFindPushTarget(enemy, targetNode);
	if (found)
	{
		m_PendingPushEnemy = enemy;
		m_PendingPushNode = targetNode;
		return true;
	}

	ClearPendingPush();
	return false;
}

bool PlayerComponent::ConsumePushSuccess()
{
	//return ConsumeFlag(m_PushSuccess);
	const bool diceSuccess = ConsumeFlag(m_PushSuccess);
	if (!diceSuccess)
	{
		ClearPendingPush();
		return false;
	}

	if (!m_PendingPushEnemy || !m_PendingPushNode)
	{
		EnemyComponent* enemy = nullptr;
		NodeComponent* targetNode = nullptr;
		if (!TryFindPushTarget(enemy, targetNode))
		{
			ClearPendingPush();
			return false;
		}
		m_PendingPushEnemy = enemy;
		m_PendingPushNode = targetNode;
	}

	const bool success = ResolvePushTarget(m_PendingPushEnemy, m_PendingPushNode);
	ClearPendingPush();
	return success;
}

bool PlayerComponent::ConsumeDoorConfirmed()
{
	return ConsumeFlag(m_DoorConfirmed);
}

bool PlayerComponent::ConsumeDoorSuccess()
{
	return ConsumeFlag(m_DoorSuccess);
}

bool PlayerComponent::ConsumeInventoryAtShop()
{
	return ConsumeFlag(m_InventoryAtShop);
}

bool PlayerComponent::ConsumeInventoryCanDrop()
{
	return ConsumeFlag(m_InventoryCanDrop);
}

bool PlayerComponent::ConsumeShopHasSpace()
{
	return ConsumeFlag(m_ShopHasSpace);
}

bool PlayerComponent::ConsumeShopHasMoney()
{
	return ConsumeFlag(m_ShopHasMoney);
}

void PlayerComponent::SetPendingDoor(DoorComponent* door)
{
	m_PendingDoor = door;
}

DoorComponent* PlayerComponent::ConsumePendingDoor()
{
	DoorComponent* door = m_PendingDoor;
	m_PendingDoor = nullptr;
	return door;
}

void PlayerComponent::ResetSubFSMFlags()
{
	m_PushPossible = true;
	m_PushTargetFound = true;
	m_PushSuccess = true;
	m_DoorConfirmed = true;
	m_DoorSuccess = true;
	m_InventoryAtShop = true;
	m_InventoryCanDrop = true;
	m_ShopHasSpace = true;
	m_ShopHasMoney = true;

	m_PendingDoor = nullptr;
	ClearPendingPush();
}

bool PlayerComponent::ConsumeFlag(bool& flag)
{
	const bool value = flag;
	flag = true;
	return value;
}

bool PlayerComponent::TryGetConsumableThrowRange(int& outRange) const
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	int bestRange = -1;
	for (const auto& itemName : m_ConsumableItemNames) 
	{
		auto* itemObject = FindGameObjectByName(scene, itemName);
		if (!itemObject)
		{
			continue;
		}

		const auto* itemComponent = itemObject->GetComponent<ItemComponent>();
		if (!itemComponent)
		{
			continue;
		}

		if (itemComponent->GetType() != static_cast<int>(ItemType::THROW))
		{
			continue;
		}

		bestRange = max(bestRange, itemComponent->GetThrowRange());
	}

	if (bestRange <= 0)
	{
		return false;
	}

	outRange = bestRange;
	return true;
}

bool PlayerComponent::TryGetConsumableThrowItem(ItemComponent*& outItem) const
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	outItem = nullptr;
	int bestRange = -1;
	for (const auto& itemName : m_ConsumableItemNames)
	{
		auto* itemObject = FindGameObjectByName(scene, itemName);
		if (!itemObject)
		{
			continue;
		}

		auto* itemComponent = itemObject->GetComponent<ItemComponent>();
		if (!itemComponent)
		{
			continue;
		}

		if (itemComponent->GetType() != static_cast<int>(ItemType::THROW))
		{
			continue;
		}

		const int range = itemComponent->GetThrowRange();
		if (range > bestRange)
		{
			bestRange = range;
			outItem = itemComponent;
		}
	}

	return outItem != nullptr && bestRange > 0;
}

bool PlayerComponent::ApplyThrowDamage(ItemComponent* throwItem, EnemyComponent* enemy)
{
	if (!throwItem || !enemy)
	{
		return false;
	}

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return false;
	}

	auto* enemyOwner = enemy->GetOwner();
	auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
	if (!enemyStat)
	{
		return false;
	}

	auto& services = scene->GetServices();
	if (!services.Has<DiceSystem>())
	{
		return false;
	}

	auto& diceSystem = services.Get<DiceSystem>();
	const int diceCount = max(0, throwItem->GetDiceRoll());
	const int diceSides = max(0, throwItem->GetDiceType());
	const int bonus = max(0, throwItem->GetBaseModifier());
	int damage = bonus;

	if (diceCount > 0 && diceSides > 0)
	{
		const DiceConfig rollConfig{ diceCount, diceSides, 0 };
		damage += diceSystem.RollTotal(rollConfig, RandomDomain::World);
	}

	if (damage <= 0)
	{
		return false;
	}

	const int prevHp = enemyStat->GetCurrentHP();
	const int nextHp = max(0, prevHp - damage);
	enemyStat->SetCurrentHP(nextHp);
	std::cout << "[Throw] Damage=" << damage << " Enemy HP: " << prevHp << " -> " << nextHp << std::endl;
	return true;
}

void PlayerComponent::ConsumeThrowItem(ItemComponent* throwItem)
{
	if (!throwItem)
	{
		return;
	}

	auto* itemOwner = throwItem->GetOwner();
	const std::string itemName = itemOwner ? itemOwner->GetName() : std::string{};
	for (auto& slotName : m_ConsumableItemNames)
	{
		if (!slotName.empty() && slotName == itemName)
		{
			slotName.clear();
			break;
		}
	}

	if (itemOwner)
	{
		const std::string& itemName = itemOwner->GetName();
		auto it = std::remove(m_InventoryItemIds.begin(), m_InventoryItemIds.end(), itemName);
		if (it != m_InventoryItemIds.end())
		{
			m_InventoryItemIds.erase(it, m_InventoryItemIds.end());
		}
	}

	throwItem->SetIsEquiped(false);

	int range = 0;
	if (TryGetConsumableThrowRange(range))
	{
		m_ThrowPreviewRange = range;
		if (m_GridSystem)
		{
			m_GridSystem->SetThrowRangePreview(true, range);
		}
	}
	else
	{
		EndThrowPreview();
	}
}

void PlayerComponent::BeginThrowPreview()
{
	if (m_IsThrowPreviewActive)
	{
		return;
	}

	if (!m_GridSystem)
	{
		return;
	}

	int range = 0;
	if (!TryGetConsumableThrowRange(range))
	{
		return;
	}

	m_IsThrowPreviewActive = true;
	m_ThrowPreviewRange = range;
	m_GridSystem->SetThrowRangePreview(true, range);
}

void PlayerComponent::EndThrowPreview()
{
	if (!m_IsThrowPreviewActive)
	{
		return;
	}

	m_IsThrowPreviewActive = false;
	m_ThrowPreviewRange = 0;
	if (m_GridSystem)
	{
		m_GridSystem->SetThrowRangePreview(false, 0);
	}
}

bool PlayerComponent::TryPickup(ItemComponent* item)
{
	if (!item)
	{
		return false;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	auto* itemOwner = item->GetOwner();
	auto* itemObject = itemOwner ? dynamic_cast<GameObject*>(itemOwner) : nullptr;
	if (!itemObject)
	{
		return false;
	}

	//획득 반경
	constexpr float kPickupRadius = 1.5f;
	auto* itemTransform = itemObject->GetComponent<TransformComponent>();
	auto* playerTransform = owner->GetComponent<TransformComponent>();
	if (itemTransform && playerTransform)
	{
		const float distSq = DistanceSq2D(playerTransform->GetPosition(), itemTransform->GetPosition());
		if (distSq > kPickupRadius * kPickupRadius)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerEquipFailed, item);
			return false;
		}
	}

	const int itemType = item->GetType();
	int consumableSlot = -1;
	if (itemType == static_cast<int>(ItemType::EQUIPMENT))
	{
		if (m_MeeleItem)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerEquipFailed, item);
			return false;
		}
	}
	else if (itemType == static_cast<int>(ItemType::HEAL) || itemType == static_cast<int>(ItemType::THROW))
	{
		for (int i = 0; i < 3; ++i)
		{
			if (m_ConsumableItemNames[i].empty()) 
			{
				consumableSlot = i;
				break;
			}
		}

		if (consumableSlot < 0)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerEquipFailed, item);
			return false;
		}
	}

	if (!item->RequestPickup(owner))
	{
		return false;
	}

	AddToInventory(item);
	if (itemType == static_cast<int>(ItemType::EQUIPMENT))
	{
		m_MeeleItem = itemObject;
		item->SetIsEquiped(true);
	}
	else if (consumableSlot >= 0)
	{
		m_ConsumableItemNames[consumableSlot] = itemObject->GetName();
	}

	item->CompletePickup(owner);
	return true;
}
void PlayerComponent::AddToInventory(ItemComponent * item)
{
	if (!item)
	{
		return;
	}

	auto* itemOwner = item->GetOwner();
	if (!itemOwner)
	{
		return;
	}

	m_InventoryItemIds.push_back(itemOwner->GetName());
}
