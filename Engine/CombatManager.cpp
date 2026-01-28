#include "pch.h"
#include "CombatManager.h"
#include "AIController.h"
#include "CombatResolver.h"
#include "DiceSystem.h"
#include "LogSystem.h"
#include "EventDispatcher.h"
#include "CombatEvents.h"


CombatManager::CombatManager(CombatResolver& combatResolver, DiceSystem& diceSystem, LogSystem* logger)
    : m_Resolver(combatResolver), m_DiceSystem(diceSystem), m_LogSystem(logger)
{
}

void CombatManager::HandlePlayerAttack(const AttackRequest& request)
{
    if (!CanAct(request.actorId))
        return;

    if (request.targetIds.empty())
        return;

    if (m_State == Battle::NonBattle)
    {
        EnterBattle(request.actorId, request.targetIds.front());
    }

    AdvanceTurn();
}

void CombatManager::TickAI(AIController& controller, float deltaTime)
{
    if (m_State != Battle::InBattle)
    {
        return;
    }

    controller.Tick(deltaTime);
}

void CombatManager::EnterBattle(int initiatorId, int targetId)
{
    (void)initiatorId;
    (void)targetId;

    m_State = Battle::InBattle;
	if (m_EventDispatcher)
	{
		const CombatEnterEvent eventData{ initiatorId, targetId };
		m_EventDispatcher->Dispatch(EventType::CombatEnter, &eventData);
	}

    BuildInitiativeOrder();
    m_CurrentTurnIndex = 0;
}

void CombatManager::ExitBattle()
{
    m_State = Battle::NonBattle;
    m_InitiativeOrder.clear();
    m_CurrentTurnIndex = 0;

	if (m_EventDispatcher)
	{
		const CombatExitEvent eventData;
		m_EventDispatcher->Dispatch(EventType::CombatExit, &eventData);
	}
}

void CombatManager::SetCombatants(const std::vector<CombatantSnapshot>& combatants)
{
	m_Combatants = combatants;
}

void CombatManager::UpdateBattleOutcome(bool playerAlive, bool enemiesRemaining)
{
    if (m_State != Battle::InBattle)
        return;

    if (!playerAlive || !enemiesRemaining)
        ExitBattle();
}

void CombatManager::BuildInitiativeOrder()
{
    m_InitiativeOrder.clear();

    if (m_Combatants.empty())
        return;

    std::vector<InitiativeEntry> entries;
    entries.reserve(m_Combatants.size());

    for (const CombatantSnapshot& combatant : m_Combatants)
    {
        const DiceConfig rollConfig{ 1, 20, 0 };
        const int roll = m_DiceSystem.RollTotal(rollConfig, RandomDomain::Combat);
        entries.push_back({combatant.actorId, roll + combatant.initiativeBonus});
    }

    std::sort(entries.begin(), entries.end(),
        [](const InitiativeEntry& left, const InitiativeEntry& right)
        {
            return left.initiative > right.initiative;
        });

    for (const InitiativeEntry& entry : entries)
    {
        m_InitiativeOrder.push_back(entry.actorId);
    }

	if (m_EventDispatcher)
	{
		const CombatInitiativeBuiltEvent eventData{ &m_InitiativeOrder };
		m_EventDispatcher->Dispatch(EventType::CombatInitiativeBuilt, &eventData);
	}
}

bool CombatManager::CanAct(int actorId) const
{
    if (m_InitiativeOrder.empty())
        return false;

    return m_InitiativeOrder[m_CurrentTurnIndex] == actorId;
}

void CombatManager::AdvanceTurn()
{
    if (m_InitiativeOrder.empty())
        return;

    m_CurrentTurnIndex = (m_CurrentTurnIndex + 1) % m_InitiativeOrder.size();
}
