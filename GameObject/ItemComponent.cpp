#include "ReflectionMacro.h"

#include "Scene.h"
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
REGISTER_PROPERTY(ItemComponent, Price)
REGISTER_PROPERTY(ItemComponent, MeleeAttackRange)
REGISTER_PROPERTY(ItemComponent, DiceRoll)
REGISTER_PROPERTY(ItemComponent, DiceType)
REGISTER_PROPERTY(ItemComponent, BaseModifier)
REGISTER_PROPERTY(ItemComponent, Health)
REGISTER_PROPERTY(ItemComponent, Strength)
REGISTER_PROPERTY(ItemComponent, Agility)
REGISTER_PROPERTY(ItemComponent, Sense)
REGISTER_PROPERTY(ItemComponent, Skill)
REGISTER_PROPERTY(ItemComponent, DEF)
REGISTER_PROPERTY(ItemComponent, ThrowRange)
REGISTER_PROPERTY(ItemComponent, DifficultyGroup)
REGISTER_PROPERTY(ItemComponent, EquipmentBindPose)


ItemComponent::ItemComponent()
{
}

ItemComponent::~ItemComponent()
{
}

void ItemComponent::Start()
{
	XMStoreFloat4x4(&m_EquipmentBindPose, XMMatrixIdentity());

}

void ItemComponent::Update(float deltaTime)
{
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
	
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene || scene->GetIsPause())
	{
		return;
	}

	SelfRotate(deltaTime);
	SelfBob(deltaTime);
}

void ItemComponent::OnEvent(EventType type, const void* data)
{
}

void ItemComponent::SelfRotate(float deltaTime)
{
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

	XMFLOAT4 currentRot = transform->GetRotation();
	XMVECTOR currentQuat = XMLoadFloat4(&currentRot);
	XMVECTOR deltaQuat = XMQuaternionRotationRollPitchYaw(0.0f, deltaTime, 0.0f);
	XMVECTOR nextQuat = XMQuaternionNormalize(XMQuaternionMultiply(currentQuat, deltaQuat));

	XMStoreFloat4(&currentRot, nextQuat);
	transform->SetRotation(currentRot);
}

void ItemComponent::SelfBob(float dTime)
{
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

	m_BobTime += dTime;

	XMFLOAT3 pos = transform->GetPosition();

	const float bobHeight = 0.001f;   // 움직이는 높이
	const float bobSpeed = 2.0f;    // 속도

	pos.y += sinf(m_BobTime * bobSpeed) * bobHeight;

	transform->SetPosition(pos);
}
