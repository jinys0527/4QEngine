#pragma once

#include <vector>
#include <cstddef>

#include "GameManager.h"

class EventDispatcher;
class DiceSystem;
class LogSystem;
class CombatResolver;
class AIController;

enum class AttackAreaType
{
	SingleTarget,
	Radius,
	Cone
};

enum class AttackType
{
	Melee,
	Throw
};

struct AttackRequest
{
	int actorId = 0;
	std::vector<int> targetIds;
	AttackAreaType areaType = AttackAreaType::SingleTarget;
	float radius    = 0.0f;
	float coneAngle = 0.0f;
	AttackType attackType = AttackType::Melee;
};

struct CombatantSnapshot
{
	int  actorId		 = 0;
	int  initiativeBonus = 0;
	bool isPlayer		 = false;
};

struct InitiativeEntry
{
	int actorId = 0;
	int initiative = 0;
};

class CombatManager
{
public:
	CombatManager(CombatResolver& combatResolver,
				  DiceSystem& diceSystem,
				  LogSystem* logger = nullptr);

	void HandlePlayerAttack(const AttackRequest& request);
	void TickAI(AIController& controller, float deltaTime);
	void EnterBattle(int initiatorId, int targetId);
	void ExitBattle();
	void SetCombatants(const std::vector<CombatantSnapshot>& combatants);
	void UpdateBattleOutcome(bool playerAlive, bool enemiesRemaining);
	void SetEventDispatcher(EventDispatcher* dispatcher) { m_EventDispatcher = dispatcher; }

	BattleCheck GetState() const { return m_State; }

private:
	void BuildInitiativeOrder();
	bool CanAct(int actorId) const;
	void AdvanceTurn();

	BattleCheck		 m_State = BattleCheck::NonBattle;
	std::vector<CombatantSnapshot> m_Combatants;
	std::vector<int> m_InitiativeOrder;
	std::size_t		 m_CurrentTurnIndex = 0;

	CombatResolver&  m_Resolver;
	DiceSystem&      m_DiceSystem;
	LogSystem*       m_LogSystem = nullptr;
	EventDispatcher* m_EventDispatcher = nullptr;
};

