#include "ReflectionMacro.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

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
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"
#include "GridSystemComponent.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "BoxColliderComponent.h"
#include "json.hpp"

REGISTER_COMPONENT(ItemSpawnerComponent)
REGISTER_PROPERTY(ItemSpawnerComponent, FixedItemTemplateName)
REGISTER_PROPERTY(ItemSpawnerComponent, DropItemTemplateName)
REGISTER_PROPERTY(ItemSpawnerComponent, FixedItemIndex)
REGISTER_PROPERTY(ItemSpawnerComponent, EnemyDefinitionId)
REGISTER_PROPERTY(ItemSpawnerComponent, DropTableGroupOverride)
REGISTER_PROPERTY(ItemSpawnerComponent, DebugDropTrigger)
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
		item.SetActionPointCost(definition.actionPointCost);
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
		if (definition.diceRoll > 0)
		{
			item.SetDiceRoll(definition.diceRoll);
		}
		item.SetBaseModifier(definition.baseModifier);
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

		auto* boxColliderComponent = object.GetComponent<BoxColliderComponent>();
		if (!boxColliderComponent)
		{
			boxColliderComponent = object.AddComponent<BoxColliderComponent>();
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

	constexpr float kItemOverlapRatio = 0.2f;
	constexpr int kNeighborCount = 6;
	constexpr int kNeighborOffsets[kNeighborCount][2] = {
		{ 1, 0 },
		{ 1, -1 },
		{ 0, -1 },
		{ -1, 0 },
		{ -1, 1 },
		{ 0, 1 }
	};

	struct VertexCandidate
	{
		XMFLOAT3 position{};
		float distanceSq = 0.0f;
		int vertexIndex = 0;
	};

	float DistanceSq2D(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;
		return dx * dx + dz * dz;
	}

	GridSystemComponent* FindGridSystem(const Scene& scene)
	{
		const auto& objects = scene.GetGameObjects();
		for (const auto& [name, object] : objects)
		{
			if (!object)
			{
				continue;
			}
			if (auto* grid = object->GetComponent<GridSystemComponent>())
			{
				return grid;
			}
		}
		return nullptr;
	}

	std::optional<XMFLOAT3> FindPlayerPosition(const Scene& scene)
	{
		const auto& objects = scene.GetGameObjects();
		for (const auto& [name, object] : objects)
		{
			if (!object)
			{
				continue;
			}
			if (auto* player = object->GetComponent<PlayerComponent>())
			{
				if (auto* transform = object->GetComponent<TransformComponent>())
				{
					return transform->GetWorldPos();
				}
			}
		}
		return std::nullopt;
	}

	bool HasItemAtPosition(const Scene& scene, const XMFLOAT3& position, float threshold)
	{
		const float thresholdSq = threshold * threshold;
		const auto& objects = scene.GetGameObjects();
		for (const auto& [name, object] : objects)
		{
			if (!object)
			{
				continue;
			}
			if (!object->GetComponent<ItemComponent>())
			{
				continue;
			}
			auto* transform = object->GetComponent<TransformComponent>();
			if (!transform)
			{
				continue;
			}
			if (DistanceSq2D(transform->GetWorldPos(), position) <= thresholdSq)
			{
				return true;
			}
		}
		return false;
	}

	float EstimateInnerRadius(NodeComponent* centerNode)
	{
		if (!centerNode)
		{
			return 1.0f;
		}
		auto* centerOwner = centerNode->GetOwner();
		auto* centerTransform = centerOwner ? centerOwner->GetComponent<TransformComponent>() : nullptr;
		if (!centerTransform)
		{
			return 1.0f;
		}
		for (auto* neighbor : centerNode->GetNeighbors())
		{
			if (!neighbor)
			{
				continue;
			}
			auto* neighborOwner = neighbor->GetOwner();
			auto* neighborTransform = neighborOwner ? neighborOwner->GetComponent<TransformComponent>() : nullptr;
			if (!neighborTransform)
			{
				continue;
			}
			const float dist = std::sqrt(DistanceSq2D(centerTransform->GetPosition(), neighborTransform->GetPosition()));
			if (dist > 0.0f)
			{
				return dist * 0.5f;
			}
		}
		return 1.0f;
	}

	std::optional<XMFLOAT3> FindValidVertexAroundNode(
		const Scene& scene,
		GridSystemComponent& grid,
		const AxialKey& centerKey,
		const XMFLOAT3& centerPos,
		const std::optional<XMFLOAT3>& preferredPos)
	{
		auto* centerNode = grid.GetNodeByKey(centerKey);
		const float innerRadius = EstimateInnerRadius(centerNode);
		const float outerRadius = innerRadius * 2.0f / std::sqrt(3.0f);
		const float itemOverlapThreshold = outerRadius * kItemOverlapRatio;
		
		std::vector<VertexCandidate> candidates;
		candidates.reserve(kNeighborCount);

		for (int i = 0; i < kNeighborCount; ++i)
		{
			const float angle = (60.0f * static_cast<float>(i) - 30.0f) * (3.1415926535f / 180.0f);
			const XMFLOAT3 vertexPos{
				centerPos.x + outerRadius * std::cos(angle),
				centerPos.y,
				centerPos.z + outerRadius * std::sin(angle)
			};

			const int prevIndex = (i + kNeighborCount - 1) % kNeighborCount;
			const AxialKey neighborA{ centerKey.q + kNeighborOffsets[i][0], centerKey.r + kNeighborOffsets[i][1] };
			const AxialKey neighborB{ centerKey.q + kNeighborOffsets[prevIndex][0], centerKey.r + kNeighborOffsets[prevIndex][1] };

			NodeComponent* nodesToCheck[3] = {
				centerNode,
				grid.GetNodeByKey(neighborA),
				grid.GetNodeByKey(neighborB)
			};

			bool valid = true;
			for (auto* node : nodesToCheck)
			{
				if (!node || !node->GetIsMoveable())
				{
					valid = false;
					break;
				}
			}
			if (!valid)
			{
				continue;
			}

			float distanceSq = 0.0f;
			if (preferredPos)
			{
				distanceSq = DistanceSq2D(*preferredPos, vertexPos);
			}

			candidates.push_back(VertexCandidate{ vertexPos, distanceSq, i });
		}

		if (candidates.empty())
		{
			return std::nullopt;
		}

		if (preferredPos)
		{
			std::sort(candidates.begin(), candidates.end(),
				[](const VertexCandidate& a, const VertexCandidate& b)
				{
					return a.distanceSq < b.distanceSq;
				});
		}

		for (const auto& candidate : candidates)
		{
			if (HasItemAtPosition(scene, candidate.position, itemOverlapThreshold))
			{
				continue;
			}
			return candidate.position;
		}

		return std::nullopt;
	}

	std::optional<XMFLOAT3> FindDropVertexPosition(const Object& owner, const Scene& scene)
	{
		auto* ownerTransform = owner.GetComponent<TransformComponent>();
		if (!ownerTransform)
		{
			return std::nullopt;
		}

		auto* grid = FindGridSystem(scene);
		if (!grid)
		{
			return ownerTransform->GetWorldPos();
		}

		auto* enemy = owner.GetComponent<EnemyComponent>();
		if (!enemy)
		{
			return ownerTransform->GetWorldPos();
		}

		const AxialKey centerKey{ enemy->GetQ(), enemy->GetR() };
		const XMFLOAT3 centerPos = ownerTransform->GetWorldPos();
		const auto playerPos = FindPlayerPosition(scene);
		return FindValidVertexAroundNode(scene, *grid, centerKey, centerPos, playerPos);
	}

	constexpr int kVendingSearchMaxDepth = 3;

	AxialKey FindNearestNodeKey(const GridSystemComponent& grid, const XMFLOAT3& position)
	{
		const auto& nodes = grid.GetNodes();
		float bestDistance = (std::numeric_limits<float>::max)();
		AxialKey bestKey{};
		bool found = false;

		for (auto* node : nodes)
		{
			if (!node)
			{
				continue;
			}
			auto* owner = node->GetOwner();
			auto* transform = owner ? owner->GetComponent<TransformComponent>() : nullptr;
			if (!transform)
			{
				continue;
			}

			const float dist = DistanceSq2D(position, transform->GetWorldPos());
			if (!found || dist < bestDistance)
			{
				found = true;
				bestDistance = dist;
				bestKey = AxialKey{ node->GetQ(), node->GetR() };
			}
		}

		return bestKey;
	}

	bool IsSpawnableNode(const Scene& scene, GridSystemComponent& grid, const AxialKey& key, float itemOverlapThreshold)
	{
		auto* node = grid.GetNodeByKey(key);
		if (!node || !node->GetIsMoveable() || node->GetState() != NodeState::Empty)
		{
			return false;
		}

		auto* nodeOwner = node->GetOwner();
		auto* nodeTransform = nodeOwner ? nodeOwner->GetComponent<TransformComponent>() : nullptr;
		if (!nodeTransform)
		{
			return false;
		}

		return !HasItemAtPosition(scene, nodeTransform->GetWorldPos(), itemOverlapThreshold);
	}

	std::optional<AxialKey> FindSpawnableKeyByExpansion(const Scene& scene,
		GridSystemComponent& grid,
		const AxialKey& start,
		float itemOverlapThreshold,
		int maxDepth)
	{
		std::queue<std::pair<AxialKey, int>> frontier;
		std::unordered_set<long long> visited;

		auto makeHash = [](const AxialKey& key)
			{
				return (static_cast<long long>(key.q) << 32) ^ static_cast<unsigned int>(key.r);
			};

		frontier.push({ start, 0 });
		visited.insert(makeHash(start));

		while (!frontier.empty())
		{
			auto [current, depth] = frontier.front();
			frontier.pop();

			if (IsSpawnableNode(scene, grid, current, itemOverlapThreshold))
			{
				return current;
			}

			if (depth >= maxDepth)
			{
				continue;
			}

			for (int i = 0; i < kNeighborCount; ++i)
			{
				AxialKey neighbor{ current.q + kNeighborOffsets[i][0], current.r + kNeighborOffsets[i][1] };
				const long long hash = makeHash(neighbor);
				if (visited.find(hash) != visited.end())
				{
					continue;
				}
				visited.insert(hash);
				frontier.push({ neighbor, depth + 1 });
			}
		}

		return std::nullopt;
	}

	std::optional<XMFLOAT3> FindVendingDropPosition(const Object& owner,
		const Scene& scene,
		const PlayerComponent& player,
		bool& hasLastKey,
		AxialKey& lastKey)
	{
		auto* ownerTransform = owner.GetComponent<TransformComponent>();
		if (!ownerTransform)
		{
			return std::nullopt;
		}

		auto* grid = FindGridSystem(scene);
		if (!grid)
		{
			return ownerTransform->GetWorldPos();
		}

		auto* playerObject = player.GetOwner();
		auto* playerTransform = playerObject ? playerObject->GetComponent<TransformComponent>() : nullptr;
		if (!playerTransform)
		{
			return std::nullopt;
		}

		const AxialKey playerKey{ player.GetQ(), player.GetR() };
		const AxialKey vendingKey = FindNearestNodeKey(*grid, ownerTransform->GetWorldPos());
		const AxialKey midpoint{ (playerKey.q + vendingKey.q) / 2, (playerKey.r + vendingKey.r) / 2 };

		float innerRadius = 1.0f;
		if (auto* midNode = grid->GetNodeByKey(midpoint))
		{
			innerRadius = EstimateInnerRadius(midNode);
		}
		const float outerRadius = innerRadius * 2.0f / std::sqrt(3.0f);
		const float itemOverlapThreshold = outerRadius * kItemOverlapRatio;

		std::optional<AxialKey> chosen;
		if (hasLastKey)
		{
			chosen = FindSpawnableKeyByExpansion(scene, *grid, lastKey, itemOverlapThreshold, 1);
		}
		if (!chosen)
		{
			chosen = FindSpawnableKeyByExpansion(scene, *grid, midpoint, itemOverlapThreshold, kVendingSearchMaxDepth);
		}
		if (!chosen)
		{
			return std::nullopt;
		}

		auto* spawnNode = grid->GetNodeByKey(*chosen);
		auto* spawnOwner = spawnNode ? spawnNode->GetOwner() : nullptr;
		auto* spawnTransform = spawnOwner ? spawnOwner->GetComponent<TransformComponent>() : nullptr;
		if (!spawnTransform)
		{
			return std::nullopt;
		}

		const std::optional<XMFLOAT3> preferredPos = playerTransform ? std::optional<XMFLOAT3>(playerTransform->GetWorldPos()) : std::nullopt;
		const std::optional<XMFLOAT3> vertexPos = FindValidVertexAroundNode(scene, *grid, *chosen, spawnTransform->GetWorldPos(), preferredPos);

		hasLastKey = true;
		lastKey = *chosen;

		if (vertexPos)
		{
			return vertexPos;
		}

		return spawnTransform->GetWorldPos();
	}

	void FinalizeSpawn(
		const Object& owner,
		const std::shared_ptr<GameObject>& spawned,
		const std::optional<XMFLOAT3>& positionOverride = std::nullopt) 
	{
		if (!spawned)
		{
			return;
		}

		if (auto* spawnTransform = spawned->GetComponent<TransformComponent>())
		{
			if (positionOverride)
			{
				spawnTransform->SetPosition(*positionOverride);
			}
			else if (auto* ownerTransform = owner.GetComponent<TransformComponent>()) 
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

	if (m_DebugDropTrigger)
	{
		m_DebugDropTrigger = false;
		m_DropTriggered = false;
		DropItem();
		return;
	}

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


void ItemSpawnerComponent::EnsureDropQuantityCache(int dropTableGroup, const GameDataRepository& repository)
{
	if (dropTableGroup <= 0)
	{
		dropTableGroup = 1;
	}

	if (m_ActiveDropTableGroup == dropTableGroup && !m_RemainingDropQuantities.empty())
	{
		return;
	}

	m_ActiveDropTableGroup = dropTableGroup;
	m_RemainingDropQuantities.clear();

	const auto* table = repository.GetDropTable(dropTableGroup);
	if (!table)
	{
		return;
	}

	for (const auto& entry : table->entries)
	{
		if (entry.itemIndex <= 0)
		{
			continue;
		}

		const int quantity = (std::max)(0, static_cast<int>(std::round(entry.weight)));
		if (quantity <= 0)
		{
			continue;
		}

		m_RemainingDropQuantities[entry.itemIndex] += quantity;
	}
}

bool ItemSpawnerComponent::ConsumeDropQuantity(int itemId)
{
	auto it = m_RemainingDropQuantities.find(itemId);
	if (it == m_RemainingDropQuantities.end())
	{
		return true;
	}
	if (it->second <= 0)
	{
		return false;
	}

	--(it->second);
	return true;
}

int ItemSpawnerComponent::ResolveVendingDropTableGroup() const
{
	return m_DropTableGroupOverride > 0 ? m_DropTableGroupOverride : 1;
}


void ItemSpawnerComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

std::vector<int> ItemSpawnerComponent::PrepareVendingRandomCandidates()
{
	m_PreparedVendingRandomCandidates.clear();

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return m_PreparedVendingRandomCandidates;
	}

	auto& services = scene->GetServices();
	if (!services.Has<GameDataRepository>() || !services.Has<DiceSystem>())
	{
		return m_PreparedVendingRandomCandidates;
	}

	auto& repository = services.Get<GameDataRepository>();
	auto& diceSystem = services.Get<DiceSystem>();
	const int dropTableGroup = ResolveVendingDropTableGroup();
	EnsureDropQuantityCache(dropTableGroup, repository);

	struct WeightedItem
	{
		int itemId = 0;
		int quantity = 0;
	};

	std::vector<WeightedItem> weightedItems;
	weightedItems.reserve(m_RemainingDropQuantities.size());
	for (const auto& [itemId, remain] : m_RemainingDropQuantities)
	{
		if (remain <= 0)
		{
			continue;
		}
		const auto* definition = repository.GetItem(itemId);
		if (!definition || definition->category == ItemCategory::Currency)
		{
			continue;
		}
		weightedItems.push_back(WeightedItem{ itemId, remain });
	}

	if (weightedItems.empty())
	{
		return m_PreparedVendingRandomCandidates;
	}

	const int slotCount = (std::min)(6, static_cast<int>(weightedItems.size()));
	for (int i = 0; i < slotCount; ++i)
	{
		int totalWeight = 0;
		for (const auto& item : weightedItems)
		{
			totalWeight += item.quantity;
		}
		if (totalWeight <= 0)
		{
			break;
		}

		DiceConfig rollConfig{ 1, totalWeight, 0 };
		const int roll = diceSystem.RollTotal(rollConfig, RandomDomain::Shop);
		int cursor = 0;
		size_t pickedIndex = weightedItems.size();
		for (size_t index = 0; index < weightedItems.size(); ++index)
		{
			cursor += weightedItems[index].quantity;
			if (roll <= cursor)
			{
				pickedIndex = index;
				break;
			}
		}
		if (pickedIndex >= weightedItems.size())
		{
			break;
		}

		m_PreparedVendingRandomCandidates.push_back(weightedItems[pickedIndex].itemId);
		weightedItems.erase(weightedItems.begin() + static_cast<std::ptrdiff_t>(pickedIndex));
	}

	return m_PreparedVendingRandomCandidates;
}

int ItemSpawnerComponent::GetRemainingDropQuantity(int itemId) const
{
	auto it = m_RemainingDropQuantities.find(itemId);
	if (it == m_RemainingDropQuantities.end())
	{
		return 0;
	}

	return (std::max)(0, it->second);
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
	
	const EnemyDefinition* enemyDefinition = nullptr;
	EnemyDefinition fallbackDefinition{};
	if (m_EnemyDefinitionId > 0)
	{
		enemyDefinition = repository.GetEnemy(m_EnemyDefinitionId);
	}

	if (!enemyDefinition)
	{
		auto* stat = owner->GetComponent<EnemyStatComponent>();
		if (!stat && m_DropTableGroupOverride <= 0)
		{
			return;
		}
		fallbackDefinition.name = owner->GetName();
		if (stat)
		{
			fallbackDefinition.difficultyGroup = stat->GetDifficultyGroup();
			fallbackDefinition.dropTableGroup = stat->GetDifficultyGroup();
		}
		else if (m_DropTableGroupOverride > 0)
		{
			fallbackDefinition.difficultyGroup = m_DropTableGroupOverride;
			fallbackDefinition.dropTableGroup = m_DropTableGroupOverride;
		}
		enemyDefinition = &fallbackDefinition;
	}
	else
	{
		fallbackDefinition = *enemyDefinition;
		enemyDefinition = &fallbackDefinition;
	}

	if (m_DropTableGroupOverride > 0)
	{
		fallbackDefinition.dropTableGroup = m_DropTableGroupOverride;
	}

	const int dropTableGroup = enemyDefinition->dropTableGroup > 0 ? enemyDefinition->dropTableGroup : 1;
	EnsureDropQuantityCache(dropTableGroup, repository);
	const auto* table = repository.GetDropTable(dropTableGroup);
	if (!table || table->entries.empty())
	{
		m_DropTriggered = true;
		return;
	}

	std::vector<const DropEntry*> selectableEntries;
	selectableEntries.reserve(table->entries.size());
	for (const auto& entry : table->entries)
	{
		if (entry.itemIndex <= 0)
		{
			continue;
		}
		auto it = m_RemainingDropQuantities.find(entry.itemIndex);
		if (it == m_RemainingDropQuantities.end() || it->second <= 0)
		{
			continue;
		}
		selectableEntries.push_back(&entry);
	}

	if (selectableEntries.empty())
	{
		m_DropTriggered = true;
		return;
	}

	int totalWeight = 0;
	for (const auto* entry : selectableEntries)
	{
		totalWeight += (std::max)(1, static_cast<int>(std::round(entry->weight)));
	}
	if (totalWeight <= 0)
	{
		m_DropTriggered = true;
		return;
	}

	DiceConfig rollConfig{ 1, totalWeight, 0 };
	const int roll = diceSystem.RollTotal(rollConfig, RandomDomain::Loot);
	int cursor = 0;
	int selectedItemId = 0;
	for (const auto* entry : selectableEntries)
	{
		cursor += (std::max)(1, static_cast<int>(std::round(entry->weight)));
		if (roll <= cursor)
		{
			selectedItemId = entry->itemIndex;
			break;
		}
	}

	if (selectedItemId <= 0 || !ConsumeDropQuantity(selectedItemId))
	{
		m_DropTriggered = true;
		return;
	}

	const ItemDefinition* itemDefinition = repository.GetItem(selectedItemId);
	if (!itemDefinition)
	{
		m_DropTriggered = true;
		return;
	}

	const std::optional<XMFLOAT3> dropPosition = FindDropVertexPosition(*owner, *scene);
	if (!dropPosition)
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

	FinalizeSpawn(*owner, spawned, dropPosition);
	m_DropTriggered = true;
}

int ItemSpawnerComponent::SpawnVendingRandomItem(PlayerComponent* player, const std::vector<int>& candidateItemIds)
{
	if (!player)
	{
		return -1;
	}

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!owner || !scene)
	{
		return -1;
	}

	auto& services = scene->GetServices();
	if (!services.Has<GameDataRepository>() || !services.Has<DiceSystem>())
	{
		return -1;
	}

	auto& repository = services.Get<GameDataRepository>();
	auto& diceSystem = services.Get<DiceSystem>();

	const int dropTableGroup = ResolveVendingDropTableGroup();
	EnsureDropQuantityCache(dropTableGroup, repository);

	std::vector<int> resolvedCandidates;
	if (candidateItemIds.empty())
	{
		resolvedCandidates = PrepareVendingRandomCandidates();
	}
	else
	{
		resolvedCandidates.reserve(candidateItemIds.size());
		for (const int itemId : candidateItemIds)
		{
			if (itemId <= 0)
			{
				continue;
			}

			const auto* itemDefinition = repository.GetItem(itemId);
			if (!itemDefinition || itemDefinition->category == ItemCategory::Currency)
			{
				continue;
			}

			if (GetRemainingDropQuantity(itemId) <= 0)
			{
				continue;
			}

			resolvedCandidates.push_back(itemId);
		}
	}

	if (resolvedCandidates.empty())
	{
		return -1;
	}

	DiceConfig rollConfig{ 1, static_cast<int>(resolvedCandidates.size()), 0 };
	const int rolledIndex = diceSystem.RollTotal(rollConfig, RandomDomain::Shop) - 1;
	if (rolledIndex < 0 || rolledIndex >= static_cast<int>(resolvedCandidates.size()))
	{
		return -1;
	}

	const int rolledItemId = resolvedCandidates[static_cast<size_t>(rolledIndex)];
	const ItemDefinition* itemDefinition = repository.GetItem(rolledItemId);
	if (!itemDefinition || itemDefinition->category == ItemCategory::Currency)
	{
		return -1;
	}

	if (!ConsumeDropQuantity(rolledItemId))
	{
		return -1;
	}

	const std::optional<XMFLOAT3> dropPosition =
		FindVendingDropPosition(*owner, *scene, *player, m_HasLastVendingDropKey, m_LastVendingDropKey);
	if (!dropPosition)
	{
		return -1;
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

	FinalizeSpawn(*owner, spawned, dropPosition);
	return spawned ? rolledItemId : -1;
}