#include "CombatBehaviorServices.h"
#include "Blackboard.h"
#include "BlackboardKeys.h"
#include <cmath>

bool TryGetFloat(Blackboard& bb, const char* key, float& out)
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

	float selfX			= 0.0f;
	float selfY         = 0.0f;
	float selfZ			= 0.0f;
	float targetX		= 0.0f;
	float targetY		= 0.0f;
	float targetZ		= 0.0f;
	float sightDistance = 0.0f;
	float sightAngle    = 0.0f;
	float forwardX		= 0.0f;
	float forwardY		= 0.0f;
	float forwardZ		= 1.0f;

	if (!TryGetFloat(bb, BlackboardKeys::SelfPosX, selfX)
		|| !TryGetFloat(bb, BlackboardKeys::SelfPosY, selfY)
		|| !TryGetFloat(bb, BlackboardKeys::SelfPosZ, selfZ)
		|| !TryGetFloat(bb, BlackboardKeys::TargetPosX, targetX)
		|| !TryGetFloat(bb, BlackboardKeys::TargetPosY, targetY)
		|| !TryGetFloat(bb, BlackboardKeys::TargetPosZ, targetZ)
		|| !TryGetFloat(bb, BlackboardKeys::SightDistance, sightDistance)
		|| !TryGetFloat(bb, BlackboardKeys::SightAngle, sightAngle))
	{
		bb.Set(BlackboardKeys::HasTarget, false);
		return;
	}

	bb.TryGet(BlackboardKeys::SelfForwardX, forwardX);
	bb.TryGet(BlackboardKeys::SelfForwardY, forwardY);
	bb.TryGet(BlackboardKeys::SelfForwardZ, forwardZ);

	const float dx = targetX - selfX;
	const float dy = targetY - selfY;
	const float dz = targetZ - selfZ;
	const float distanceSq = dx * dx + dy * dy + dz * dz;
	const float distance = std::sqrt(distanceSq);

	float dot = 0.0f;
	const float forwardLen = std::sqrt(forwardX * forwardX + forwardY * forwardY + forwardZ * forwardZ);
	if (forwardLen > 0.0f && distance > 0.0f)
	{
		dot = (dx * forwardX + dy * forwardY + dz * forwardZ) / (distance * forwardLen);
	}
	const float clamped = Clamp(dot, -1.0f, 1.0f);
	const float angle = std::acos(clamped) * 180.0f / 3.1415926535f;

	bb.Set(BlackboardKeys::TargetDistance, distance);
	bb.Set(BlackboardKeys::TargetAngle, angle);
	bb.Set(BlackboardKeys::HasTarget, distance <= sightDistance && angle <= sightAngle * 0.5f);
}

void CombatStateSyncService::TickService(BTInstance& inst, Blackboard& bb, float deltaTime)
{
	(void)inst;
	(void)deltaTime;
	bool hasTarget = false;
	bb.TryGet(BlackboardKeys::HasTarget, hasTarget);
	bb.Set(BlackboardKeys::IsInCombat, hasTarget);
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
