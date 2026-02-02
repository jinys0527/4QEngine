#include "CombatBehaviorServices.h"
#include "Blackboard.h"
#include "BlackboardKeys.h"
#include <iostream>
#include <cmath>
#include "EventDispatcher.h"
#include "IEventListener.h"

bool TryGetFloat(Blackboard& bb, const char* key, float& out)
{
	return bb.TryGet(key, out);
}

bool TryGetInt(Blackboard& bb, const char* key, int& out)
{
	return bb.TryGet(key, out);
}

float Clamp(float value, float minValue, float maxValue)
{
	if (value < minValue)
		return minValue;
	if (value > maxValue)
		return maxValue;

	return value;
}

void TargetSenseService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	int selfQ = 0;
	int selfR = 0;
	int targetQ = 0;
	int targetR = 0;
	bool hasHexSightData = false;
	bool hasTargetHexLine = false;

	bool isInCombat = false;
	bb.TryGet(BlackboardKeys::IsInCombat, isInCombat);
	if (isInCombat)
	{
		std::cout << "[AI][Sense] InCombat=true -> HasTarget=true\n";
		bb.Set(BlackboardKeys::HasTarget, true);
		return;
	}


	const bool hasHexData = TryGetInt(bb, BlackboardKeys::SelfQ, selfQ)
		&& TryGetInt(bb, BlackboardKeys::SelfR, selfR)
		&& TryGetInt(bb, BlackboardKeys::TargetQ, targetQ)
		&& TryGetInt(bb, BlackboardKeys::TargetR, targetR);

	if (!hasHexData)
	{
		std::cout << "[AI][Sense] Missing hex data -> HasTarget=false\n";
		bb.Set(BlackboardKeys::HasTarget, false);
		return;
	}

	const int dq = selfQ - targetQ;
	const int dr = selfR - targetR;
	const int ds = dq + dr;
	const float distance = 0.5f * static_cast<float>(std::abs(dq) + std::abs(dr) + std::abs(ds));

	bb.Set(BlackboardKeys::TargetDistance, distance);
	bb.Set(BlackboardKeys::TargetAngle, 0.0f);

	const bool useHexSight = bb.TryGet(BlackboardKeys::HasHexSightData, hasHexSightData)
		&& hasHexSightData
		&& bb.TryGet(BlackboardKeys::HasTargetHexLine, hasTargetHexLine);
	std::cout << "[AI][Sense] useHexSight=" << useHexSight
		<< " hasTargetHexLine=" << hasTargetHexLine << " -> HasTarget="
		<< (useHexSight && hasTargetHexLine) << "\n";
	bb.Set(BlackboardKeys::HasTarget, useHexSight && hasTargetHexLine);
}

void CombatStateSyncService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;
	bool hasTarget = false;
	bb.TryGet(BlackboardKeys::HasTarget, hasTarget);

	if (hasTarget)
	{
		bb.Set(BlackboardKeys::IsInCombat, true);
	}
}

void RangeUpdateService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	float distance = 0.0f;
	float meleeRange = 1.0f;
	float throwRange = 3.0f;

	bb.TryGet(BlackboardKeys::TargetDistance, distance);
	bb.TryGet(BlackboardKeys::MeleeRange, meleeRange);
	bb.TryGet(BlackboardKeys::ThrowRange, throwRange);

	bb.Set(BlackboardKeys::InMeleeRange, distance <= meleeRange);
	bb.Set(BlackboardKeys::InThrowRange, distance <= throwRange);
	std::cout << "[AI][Range] distance=" << distance
		<< " meleeRange=" << meleeRange << " throwRange=" << throwRange
		<< " inMelee=" << (distance <= meleeRange)
		<< " inThrow=" << (distance <= throwRange) << "\n";

	// 추가: 원거리 선호면 MaintainRange 켜기
	bool preferRanged = false;
	bb.TryGet(BlackboardKeys::PreferRanged, preferRanged);
	bb.Set(BlackboardKeys::MaintainRange, preferRanged);
	std::cout << "[AI][Range] preferRanged=" << preferRanged
		<< " maintainRange=" << preferRanged << "\n";
}

void EstimatePlayerDamageService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	int maxDamage = 0;
	if (!bb.TryGet(BlackboardKeys::PlayerMaxDamage, maxDamage))
	{
		maxDamage = 1;
	}

	int hp = 0;
	if (!bb.TryGet(BlackboardKeys::HP, hp))
	{
		hp = 30;
	}

	bb.Set(BlackboardKeys::EstimatedPlayerDamage, maxDamage);
	bb.Set(BlackboardKeys::ShouldRunOff, maxDamage >= hp);
}

void RepathService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	bool hasTarget = false;
	bb.TryGet(BlackboardKeys::HasTarget, hasTarget);
	bb.Set(BlackboardKeys::PathNeedsUpdate, hasTarget);
}

AIRequestDispatchService::AIRequestDispatchService(EventDispatcher* dispatcher)
	: m_Dispatcher(dispatcher)
{
}

void AIRequestDispatchService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;

	if (!m_Dispatcher)
		return;

	// 1) Move
	bool moveRequested = false;
	if (bb.TryGet(BlackboardKeys::MoveRequested, moveRequested) && moveRequested)
	{
		std::cout << "[AI][Dispatch] MoveRequested\n";
		m_Dispatcher->Dispatch(EventType::AIMoveRequested, nullptr);
		bb.Set(BlackboardKeys::MoveRequested, false);
	}

	// 2) RunOff Move
	bool runOffMoveRequested = false;
	if (bb.TryGet(BlackboardKeys::RequestRunOffMove, runOffMoveRequested) && runOffMoveRequested)
	{
		std::cout << "[AI][Dispatch] RunOffMoveRequested\n";
		m_Dispatcher->Dispatch(EventType::AIRunOffMoveRequested, nullptr);
		bb.Set(BlackboardKeys::RequestRunOffMove, false);
	}

	// 3) Maintain Range
	bool maintainRangeRequested = false;
	if (bb.TryGet(BlackboardKeys::RequestMaintainRange, maintainRangeRequested) && maintainRangeRequested)
	{
		std::cout << "[AI][Dispatch] MaintainRangeRequested\n";
		m_Dispatcher->Dispatch(EventType::AIMaintainRangeRequested, nullptr);
		bb.Set(BlackboardKeys::RequestMaintainRange, false);
	}

	bool meleeRequested = false;
	if (bb.TryGet(BlackboardKeys::RequestMeleeAttack, meleeRequested) && meleeRequested)
	{
		std::cout << "[AI][Dispatch] MeleeAttackRequested\n";
		m_Dispatcher->Dispatch(EventType::AIMeleeAttackRequested, nullptr);
		bb.Set(BlackboardKeys::RequestMeleeAttack, false);
	}

	bool rangedRequested = false;
	if (bb.TryGet(BlackboardKeys::RequestRangedAttack, rangedRequested) && rangedRequested)
	{
		std::cout << "[AI][Dispatch] RangedAttackRequested\n";
		m_Dispatcher->Dispatch(EventType::AIRangedAttackRequested, nullptr);
		bb.Set(BlackboardKeys::RequestRangedAttack, false);
	}

	// 4) End Turn
	bool endTurnRequested = false;
	if (bb.TryGet(BlackboardKeys::EndTurnRequested, endTurnRequested) && endTurnRequested)
	{
		std::cout << "[AI][Dispatch] TurnEndRequested\n";
		m_Dispatcher->Dispatch(EventType::AITurnEndRequested, nullptr);
		bb.Set(BlackboardKeys::EndTurnRequested, false);
	}
}
