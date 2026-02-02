#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "Scene.h"
#include "GridSystemComponent.h"
#include "ItemComponent.h"
#include "PlayerStatComponent.h"
#include "SkeletalMeshComponent.h"
#include "TransformComponent.h"
#include <cmath>
#include "GameManager.h"

REGISTER_COMPONENT(PlayerComponent)
REGISTER_PROPERTY_READONLY(PlayerComponent, Q)
REGISTER_PROPERTY_READONLY(PlayerComponent, R)
REGISTER_PROPERTY(PlayerComponent, MoveResource)
REGISTER_PROPERTY(PlayerComponent, ActResource)
REGISTER_PROPERTY(PlayerComponent, CurrentWeaponCost)
REGISTER_PROPERTY(PlayerComponent, AttackRange)
REGISTER_PROPERTY(PlayerComponent, Money)
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
	GetEventDispatcher().RemoveListener(EventType::MouseLeftDoubleClick, this);
}

void PlayerComponent::Start()
{
	//start시 GameManagaer get
	ResetTurnResources();
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene) { return; }

	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftDoubleClick, this);
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

	auto* gameManager = scene->GetGameManager();
	const bool allowExplorationTurn = !gameManager && m_CurrentTurn == Turn::PlayerTurn;


	//Player Turn 종료 조건
	if (allowExplorationTurn) {
		m_TurnElapsed += deltaTime;

		if (!m_TurnEndRequested && m_TurnElapsed >= m_PlayerTurnTime) {
			//종료(턴 전환)
			GetEventDispatcher().Dispatch(EventType::PlayerTurnEndRequested, nullptr);
			m_TurnEndRequested = true;
		}
	}

	//임시로 첫번째 자식을 가지고 있는 아이템으로 지정
	auto* transformcomponent = owner->GetComponent<TransformComponent>();
	{
		if (!transformcomponent->GetChildrens().empty() && m_MeeleItem == nullptr)
		{
			GameObject* item = dynamic_cast<GameObject*>(transformcomponent->GetChildrens()[0]->GetOwner());
			auto* itemcomp = item->GetComponent<ItemComponent>();
			if (itemcomp && itemcomp->GetType() == 1)
			{
				m_MeeleItem = item;
				itemcomp->SetIsEquiped(true);
				m_InventoryItemIds.push_back(item->GetName());

			}
		}

	}

	//근접 아이템이 있으면 그 아이템에서 장착 본 행렬 넘겨주기
	//스켈레탈이 있으면 장착 본 행렬을 RenderData에 넘겨주기
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

			int dmg = CalculateDamage();

			m_IsApplyMeeleStat = true;
		}


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
	}
}

void PlayerComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::MouseLeftDoubleClick)
	{
		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;
		if (!gameManager || gameManager->IsCombatInputAllowed())
		{
			m_CombatConfirmRequested = true;
		}
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
	m_CombatConfirmRequested = false;
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

int PlayerComponent::CalculateDamage()
{
	auto* item = m_MeeleItem->GetComponent<ItemComponent>();
	if (!item) return 0;

	srand(static_cast<unsigned>(time(nullptr)));

	int roll = item->GetMaxDiceRoll();
	int value = item->GetMaxDiceValue();
	int bonus = item->GetBonusValue();

	int dmg = 0;
	for (int i = 0; i < roll; i++)
	{
		dmg += rand() % value + 1;
	}
	dmg += bonus;

	return bonus;
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
	return ConsumeFlag(m_PushPossible);
}

bool PlayerComponent::ConsumePushTargetFound()
{
	return ConsumeFlag(m_PushTargetFound);
}

bool PlayerComponent::ConsumePushSuccess()
{
	return ConsumeFlag(m_PushSuccess);
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
}

bool PlayerComponent::ConsumeFlag(bool& flag)
{
	const bool value = flag;
	flag = true;
	return value;
}