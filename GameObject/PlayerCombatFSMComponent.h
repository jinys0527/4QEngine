#pragma once
#include "FSMComponent.h"
#include <vector>

struct CombatantSnapshot;
class CombatManager;

class PlayerCombatFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerCombatFSMComponent";
	const char* GetTypeName() const override;

	PlayerCombatFSMComponent();
	virtual ~PlayerCombatFSMComponent() override;

	void Start() override;

	void SetCombatManager(CombatManager* manager) { m_CombatManager = manager; }
protected:
	std::optional<std::string> TranslateEvent(EventType type, const void* data) override;

private:
	bool EnsureCombatManager();
	void BuildCombatantSnapshots(std::vector<CombatantSnapshot>& outCombatants) const;
	bool HasEnemyInAttackRange() const;
	int  GetPlayerActorId() const;
	bool IsPlayerActor(int actorId) const;

	CombatManager* m_CombatManager = nullptr;
};

