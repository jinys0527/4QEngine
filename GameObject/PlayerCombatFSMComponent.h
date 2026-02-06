#pragma once
#include "FSMComponent.h"
#include <vector>

struct CombatantSnapshot;
class CombatManager;
class EnemyComponent;
class ItemComponent;
class PlayerComponent;

class PlayerCombatFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerCombatFSMComponent";
	const char* GetTypeName() const override;

	PlayerCombatFSMComponent();
	virtual ~PlayerCombatFSMComponent() override;

	void Start() override;

	void SetCombatManager(CombatManager* manager) { m_CombatManager = manager; }
	bool RequestCombatEnter(int initiatorId, int targetId);
	bool TryExecutePlayerAttackFromInput();
	bool TryExecutePlayerThrowAttack(EnemyComponent* enemy);

protected:
	std::optional<std::string> TranslateEvent(EventType type, const void* data) override;

private:
	bool EnsureCombatManager();
	bool ExecutePlayerAttack();
	bool ResolvePlayerAttackMode(PlayerComponent& player, int& outRange, ItemComponent*& outThrowItem, bool& outIsThrow) const;
	bool ExecuteThrowAttack(PlayerComponent& player, EnemyComponent* enemy, ItemComponent* throwItem);
	bool ApplyThrowDamage(ItemComponent* throwItem, EnemyComponent* enemy) const;
	void BuildCombatantSnapshots(std::vector<CombatantSnapshot>& outCombatants) const;
	bool HasEnemyInAttackRange() const;
	int  GetPlayerActorId() const;
	bool IsPlayerActor(int actorId) const;

	CombatManager* m_CombatManager = nullptr;
};

