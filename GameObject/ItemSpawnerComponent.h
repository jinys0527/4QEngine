#pragma once
#include "Component.h"
#include "GridSystemComponent.h"
#include <string>
#include <unordered_map>
#include <vector>

class GameObject;
class GameDataRepository;
class PlayerComponent;

class ItemSpawnerComponent : public Component
{
public:
	static constexpr const char* StaticTypeName = "ItemSpawnerComponent";
	const char* GetTypeName() const override;

	ItemSpawnerComponent() = default;
	virtual ~ItemSpawnerComponent() = default;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SpwanFixedItem();
	void DropItem();

	bool SpawnVendingRandomItem(PlayerComponent* player, const std::vector<int>& candidateItemIds);
	std::vector<int> PrepareVendingRandomCandidates();
	const std::vector<int>& GetPreparedVendingRandomCandidates() const { return m_PreparedVendingRandomCandidates; }
	int  GetRemainingDropQuantity(int itemId) const;

	const std::string& GetFixedItemTemplateName() const { return m_FixedItemTemplateName; }
	void SetFixedItemTemplateName(const std::string& value) { m_FixedItemTemplateName = value; }

	const std::string& GetDropItemTemplateName() const { return m_DropItemTemplateName; }
	void SetDropItemTemplateName(const std::string& value) { m_DropItemTemplateName = value; }

	const int& GetFixedItemIndex() const { return m_FixedItemIndex; }
	void SetFixedItemIndex(const int& value) { m_FixedItemIndex = value; }

	const int& GetEnemyDefinitionId() const { return m_EnemyDefinitionId; }
	void SetEnemyDefinitionId(const int& value) { m_EnemyDefinitionId = value; }

	const int& GetDropTableGroupOverride() const { return m_DropTableGroupOverride; }
	void SetDropTableGroupOverride(const int& value) { m_DropTableGroupOverride = value; }

	const bool& GetDebugDropTrigger() const { return m_DebugDropTrigger; }
	void SetDebugDropTrigger(const bool& value) { m_DebugDropTrigger = value; }

	const bool& GetDropOnDeath() const { return m_DropOnDeath; }
	void SetDropOnDeath(const bool& value) { m_DropOnDeath = value; }

private:
	void EnsureDropQuantityCache(int dropTableGroup, const GameDataRepository& repository);
	bool ConsumeDropQuantity(int itemId);
	int  ResolveVendingDropTableGroup() const;

	GameObject* m_SpawnItem = nullptr;

	std::string m_FixedItemTemplateName;
	std::string m_DropItemTemplateName;
	int m_FixedItemIndex = -1;
	int m_EnemyDefinitionId = 0;
	int m_DropTableGroupOverride = 0;
	bool m_DebugDropTrigger = false;
	bool m_DropOnDeath = true;
	bool m_FixedItemSpawned = false;
	bool m_DropTriggered = false;

	bool m_HasLastVendingDropKey = false;
	AxialKey m_LastVendingDropKey{};
	std::vector<int> m_PreparedVendingRandomCandidates;
	int m_ActiveDropTableGroup = -1;
	std::unordered_map<int, int> m_RemainingDropQuantities;
};
