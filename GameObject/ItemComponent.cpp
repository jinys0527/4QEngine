#include "ReflectionMacro.h"

#include "AssetLoader.h"
#include "Object.h"
#include "SkeletalMeshComponent.h"
#include "TransformComponent.h"


#include "ItemComponent.h"


REGISTER_COMPONENT(ItemComponent)
REGISTER_PROPERTY(ItemComponent, ItemIndex)
REGISTER_PROPERTY(ItemComponent, IsEquiped)
REGISTER_PROPERTY(ItemComponent, Type)
REGISTER_PROPERTY(ItemComponent, Name)
REGISTER_PROPERTY(ItemComponent, IconPath)
REGISTER_PROPERTY(ItemComponent, MeshPath)
REGISTER_PROPERTY(ItemComponent, DescriptionIndex)
REGISTER_PROPERTY(ItemComponent, SellPrice)
REGISTER_PROPERTY(ItemComponent, MeleeAttackRange)
REGISTER_PROPERTY(ItemComponent, MaxDiceRoll)
REGISTER_PROPERTY(ItemComponent, MaxDiceValue)
REGISTER_PROPERTY(ItemComponent, BonusValue)
REGISTER_PROPERTY(ItemComponent, CON)
REGISTER_PROPERTY(ItemComponent, STR)
REGISTER_PROPERTY(ItemComponent, DEX)
REGISTER_PROPERTY(ItemComponent, SENSE)
REGISTER_PROPERTY(ItemComponent, TEC)
REGISTER_PROPERTY(ItemComponent, DEF)
REGISTER_PROPERTY(ItemComponent, ThrowRange)
REGISTER_PROPERTY(ItemComponent, DifficultyGroup)
REGISTER_PROPERTY(ItemComponent, EquipmentBindPose)


ItemComponent::ItemComponent()
{
	XMStoreFloat4x4(&m_EquipmentBindPose, XMMatrixIdentity());
}

ItemComponent::~ItemComponent()
{
}

void ItemComponent::Start()
{
}

void ItemComponent::Update(float deltaTime)
{
	if (!m_IsEquiped)
	{
		return;
	}

	Object* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* transform = owner->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}


	auto* loader = AssetLoader::GetActive();
	if (!loader)
	{
		return;
	}

	//장착됨 상태라면 이 아이템을 가지는 PlayerComponent에서 행렬을 넘겨주고, 그 값을 Transform으로 적용
	if (m_IsEquiped)
	{
		XMVECTOR scale;
		XMVECTOR rotQuat;
		XMVECTOR translation;

		XMMATRIX world = XMLoadFloat4x4(&m_EquipmentBindPose);

		bool success = XMMatrixDecompose(
			&scale,
			&rotQuat,
			&translation,
			world
		);

		XMFLOAT3 pos;
		XMFLOAT3 scl;
		XMFLOAT4 rot;

		XMStoreFloat3(&pos, translation);
		XMStoreFloat3(&scl, scale);
		XMStoreFloat4(&rot, rotQuat);

		transform->SetPosition(pos);
		transform->SetRotation(rot);
		transform->SetScale(scl);

	}
	

}

void ItemComponent::OnEvent(EventType type, const void* data)
{
}