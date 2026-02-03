#include "ReflectionMacro.h"

#include <memory>
#include <string>

#include "AssetLoader.h"
#include "DiceSystem.h"
#include "GameDataRepository.h"
#include "GameObject.h"
#include "ItemComponent.h"
#include "ItemSpawnerComponent.h"
#include "LootRoller.h"
#include "LogSystem.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "TransformComponent.h"
#include "EnemyStatComponent.h"
#include "json.hpp"

REGISTER_COMPONENT(ItemSpawnerComponent)
REGISTER_PROPERTY(ItemSpawnerComponent, FixedItemTemplateName)
REGISTER_PROPERTY(ItemSpawnerComponent, DropItemTemplateName)
REGISTER_PROPERTY(ItemSpawnerComponent, FixedItemIndex)
REGISTER_PROPERTY(ItemSpawnerComponent, EnemyDefinitionId)
REGISTER_PROPERTY(ItemSpawnerComponent, DropOnDeath)

namespace
{
	int ToItemType(ItemCategory category)
	{
		switch (category)
		{
		case ItemCategory::Currency:
			return static_cast<int>(ItemType::GOLD);
		case ItemCategory::Healing:
			return static_cast<int>(ItemType::HEAL);
		case ItemCategory::Equipment:
			return static_cast<int>(ItemType::EQUIPMENT);
		case ItemCategory::Throwable:
			return static_cast<int>(ItemType::THROW);
		default:
			return static_cast<int>(ItemType::GOLD);
		}
	}

	void ApplyItemDefinition(ItemComponent& item, const ItemDefinition& definition)
	{
		item.SetItemIndex(definition.index);
		item.SetType(ToItemType(definition.category));
		item.SetIconPath(definition.iconPath);
		item.SetMeshPath(definition.meshPath);
		item.SetPrice(definition.basePrice);
		item.SetMeleeAttackRange(definition.range);
		item.SetThrowRange(definition.throwRange);
		item.SetDifficultyGroup(definition.difficultyGroup);
		item.SetHealth(definition.constitutionModifier);
		item.SetStrength(definition.strengthModifier);
		item.SetAgility(definition.agilityModifier);
		item.SetSense(definition.senseModifier);
		item.SetSkill(definition.skillModifier);
		item.SetDEF(definition.defenseBonus);
		if (definition.diceType > 0)
		{
			item.SetDiceType(definition.diceType);
		}
		if (definition.baseModifier > 0)
		{
			item.SetBaseModifier(definition.baseModifier);
		}
	}

	std::string BuildSpawnName(const std::string& base)
	{
		static int counter = 0;
		const std::string prefix = base.empty() ? "ItemSpawned" : base;
		return prefix + "_" + std::to_string(counter++);
	}

	void EnsureRenderComponents(GameObject& object, const ItemDefinition& definition)
	{
		auto* meshComponent = object.GetComponent<MeshComponent>();
		if (!meshComponent)
		{
			meshComponent = object.AddComponent<MeshComponent>();
		}

		auto* meshRenderer = object.GetComponent<MeshRenderer>();
		if (!meshRenderer)
		{
			meshRenderer = object.AddComponent<MeshRenderer>();
		}
		if (meshRenderer)
		{
			meshRenderer->SetRenderLayer(static_cast<UINT8>(RenderData::RenderLayer::OpaqueItems));
		}

		auto* materialComponent = object.GetComponent<MaterialComponent>();
		if (!materialComponent)
		{
			materialComponent = object.AddComponent<MaterialComponent>();
		}

		if (definition.meshPath.empty())
		{
			return;
		}

		auto* loader = AssetLoader::GetActive();
		if (!loader)
		{
			return;
		}

		const auto* asset = loader->GetAsset(definition.meshPath);
		if (!asset)
		{
			return;
		}

		if (meshComponent && !asset->meshes.empty())
		{
			if (!meshComponent->GetMeshHandle().IsValid())
			{
				meshComponent->SetMeshHandle(asset->meshes.front());
			}
		}

		if (materialComponent && !asset->materials.empty())
		{
			if (!materialComponent->GetMaterialHandle().IsValid())
			{
				materialComponent->SetMaterialHandle(asset->materials.front());
			}
		}
	}

	void FinalizeSpawn(const Object& owner, const std::shared_ptr<GameObject>& spawned)
	{
		if (!spawned)
		{
			return;
		}

		if (auto* ownerTransform = owner.GetComponent<TransformComponent>())
		{
			if (auto* spawnTransform = spawned->GetComponent<TransformComponent>())
			{
				spawnTransform->SetPosition(ownerTransform->GetPosition());
			}
		}

		spawned->Start();
	}
}

void ItemSpawnerComponent::Start()
{
}

void ItemSpawnerComponent::Update(float deltaTime)
{
	(void)deltaTime;

	SpwanFixedItem();

	if (!m_DropOnDeath || m_DropTriggered)
	{
		return;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* stat = owner->GetComponent<EnemyStatComponent>();
	if (!stat || stat->GetCurrentHP() > 0)
	{
		return;
	}

	DropItem();
}

void ItemSpawnerComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void ItemSpawnerComponent::SpwanFixedItem()
{
	if (m_FixedItemSpawned)
	{
		return;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* scene = owner->GetScene();
	if (!scene)
	{
		return;
	}

	std::shared_ptr<GameObject> spawned;

	if (!m_FixedItemTemplateName.empty())
	{
		const auto& objects = scene->GetGameObjects();
		auto it = objects.find(m_FixedItemTemplateName);
		if (it != objects.end() && it->second)
		{
			nlohmann::json templateJson;
			it->second->Serialize(templateJson);
			templateJson["name"] = BuildSpawnName(m_FixedItemTemplateName);
			spawned = scene->CreateGameObject(templateJson["name"].get<std::string>());
			if (spawned)
			{
				spawned->Deserialize(templateJson);
			}
		}
	}

	if (!spawned && m_FixedItemIndex >= 0)
	{
		auto& services = scene->GetServices();
		if (services.Has<GameDataRepository>())
		{
			const auto* itemDefinition = services.Get<GameDataRepository>().GetItem(m_FixedItemIndex);
			if (itemDefinition)
			{
				const std::string spawnName = BuildSpawnName(itemDefinition->name);
				spawned = scene->CreateGameObject(spawnName);
				if (spawned)
				{
					if (auto* itemComponent = spawned->AddComponent<ItemComponent>())
					{
						ApplyItemDefinition(*itemComponent, *itemDefinition);
					}

					EnsureRenderComponents(*spawned, *itemDefinition);
				}
			}
		}
	}

	if (!spawned)
	{
		return;
	}

	FinalizeSpawn(*owner, spawned);

	m_SpawnItem = spawned.get();
	m_FixedItemSpawned = true;
}

void ItemSpawnerComponent::DropItem()
{
	if (m_DropTriggered)
	{
		return;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* scene = owner->GetScene();
	if (!scene)
	{
		return;
	}

	auto& services = scene->GetServices();
	if (!services.Has<GameDataRepository>() || !services.Has<DiceSystem>())
	{
		return;
	}

	auto& repository = services.Get<GameDataRepository>();
	auto& diceSystem = services.Get<DiceSystem>();
	LogSystem* logger = services.Has<LogSystem>() ? &services.Get<LogSystem>() : nullptr;

	const EnemyDefinition* enemyDefinition = nullptr;
	EnemyDefinition fallbackDefinition{};

	if (m_EnemyDefinitionId > 0)
	{
		enemyDefinition = repository.GetEnemy(m_EnemyDefinitionId);
	}

	if (!enemyDefinition)
	{
		auto* stat = owner->GetComponent<EnemyStatComponent>();
		if (!stat)
		{
			return;
		}

		fallbackDefinition.name = owner->GetName();
		fallbackDefinition.difficultyGroup = stat->GetDifficultyGroup();
		fallbackDefinition.dropTableGroup = stat->GetDifficultyGroup();
		enemyDefinition = &fallbackDefinition;
	}

	LootRoller lootRoller;
	const std::optional<LootRollResult> result =
		lootRoller.RollDrop(*enemyDefinition, repository, diceSystem, logger);

	if (!result)
	{
		m_DropTriggered = true;
		return;
	}

	const ItemDefinition* itemDefinition = repository.GetItem(result->itemIndex);
	if (!itemDefinition)
	{
		m_DropTriggered = true;
		return;
	}

	std::shared_ptr<GameObject> spawned;
	const std::string templateName =
		m_DropItemTemplateName.empty() ? m_FixedItemTemplateName : m_DropItemTemplateName;

	if (!templateName.empty())
	{
		const auto& objects = scene->GetGameObjects();
		auto it = objects.find(templateName);
		if (it != objects.end() && it->second)
		{
			nlohmann::json templateJson;
			it->second->Serialize(templateJson);
			templateJson["name"] = BuildSpawnName(templateName);
			spawned = scene->CreateGameObject(templateJson["name"].get<std::string>());
			if (spawned)
			{
				spawned->Deserialize(templateJson);
			}
		}
	}

	if (!spawned)
	{
		const std::string spawnName = BuildSpawnName(itemDefinition->name);
		spawned = scene->CreateGameObject(spawnName);
		if (spawned)
		{
			if (auto* itemComponent = spawned->AddComponent<ItemComponent>())
			{
				ApplyItemDefinition(*itemComponent, *itemDefinition);
			}

			EnsureRenderComponents(*spawned, *itemDefinition);
		}
	}
	else
	{
		if (auto* itemComponent = spawned->GetComponent<ItemComponent>())
		{
			ApplyItemDefinition(*itemComponent, *itemDefinition);
		}
		else if (auto* itemComponent = spawned->AddComponent<ItemComponent>())
		{
			ApplyItemDefinition(*itemComponent, *itemDefinition);
		}

		EnsureRenderComponents(*spawned, *itemDefinition);
	}

	FinalizeSpawn(*owner, spawned);
	m_DropTriggered = true;
}
