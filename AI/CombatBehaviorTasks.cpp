#include "CombatBehaviorTasks.h"
#include "Blackboard.h"
#include "BlackboardKeys.h"
#include <iostream>

bool GetBool(Blackboard& bb, const char* key, bool defaultValue = false)
{
    bool value = defaultValue;
    bb.TryGet(key, value);
    return value;
}

BTStatus UpdateTargetLocationTask::OnTick(BTInstance& inst, Blackboard& bb)
{
    (void)inst;
	int targetQ = 0;
	int targetR = 0;
	if (!bb.TryGet(BlackboardKeys::TargetQ, targetQ)
		|| !bb.TryGet(BlackboardKeys::TargetR, targetR))
	{
		std::cout << "[AI][Task] UpdateTargetLocation fail: TargetQ/TargetR missing\n";
        return BTStatus::Failure;
    }

	std::cout << "[AI][Task] UpdateTargetLocation: targetQ=" << targetQ
		<< " targetR=" << targetR << "\n";
	bb.Set(BlackboardKeys::LastKnownTargetQ, targetQ);
	bb.Set(BlackboardKeys::LastKnownTargetR, targetR);
    return BTStatus::Success;
}

BTStatus MoveToTargetTask::OnTick(BTInstance& inst, Blackboard& bb)
{
    (void)inst;
    if (!GetBool(bb, BlackboardKeys::HasTarget))
    {
        return BTStatus::Failure;
    }

    bb.Set(BlackboardKeys::MoveRequested, true);
    return BTStatus::Success;
}

BTStatus ApproachTargetTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	float distance = 0.0f;
	float meleeRange = 1.0f;
	bb.TryGet(BlackboardKeys::TargetDistance, distance);
	bb.TryGet(BlackboardKeys::MeleeRange, meleeRange);

	if (distance <= meleeRange)
	{
		std::cout << "[AI][Task] ApproachTarget success: distance=" << distance
			<< " meleeRange=" << meleeRange << "\n";
		return BTStatus::Success;
	}

	std::cout << "[AI][Task] ApproachTarget move request: distance=" << distance
		<< " meleeRange=" << meleeRange << "\n";
	bb.Set(BlackboardKeys::MoveRequested, true);
	return BTStatus::Success;
}

BTStatus MeleeAttackTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	if (!GetBool(bb, BlackboardKeys::InMeleeRange))
	{
		std::cout << "[AI][Task] MeleeAttack fail: not in melee range\n";
		return BTStatus::Failure;
	}

	std::cout << "[AI][Task] MeleeAttack request\n";
	bb.Set(BlackboardKeys::RequestMeleeAttack, true);
	return BTStatus::Success;
}

BTStatus RangedAttackTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	if (!GetBool(bb, BlackboardKeys::InThrowRange))
	{
		std::cout << "[AI][Task] RangedAttack fail: not in throw range\n";
		return BTStatus::Failure;
	}

	std::cout << "[AI][Task] RangedAttack request\n";
	bb.Set(BlackboardKeys::RequestRangedAttack, true);
	return BTStatus::Success;
}

BTStatus SelectRunOffTargetTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	bool hasTarget = GetBool(bb, BlackboardKeys::HasTarget);
	bb.Set(BlackboardKeys::RunOffTargetFound, hasTarget);
	return hasTarget ? BTStatus::Success : BTStatus::Failure;
}

BTStatus RunOffMoveTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	if (!GetBool(bb, BlackboardKeys::RunOffTargetFound))
	{
		return BTStatus::Failure;
	}
	bb.Set(BlackboardKeys::RequestRunOffMove, true);
	return BTStatus::Success;
}

BTStatus MaintainRangeTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	if (!GetBool(bb, BlackboardKeys::MaintainRange))
	{
		return BTStatus::Failure;
	}
	bb.Set(BlackboardKeys::RequestMaintainRange, true);
	return BTStatus::Success;
}

BTStatus PatrolMoveTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	bb.Set(BlackboardKeys::MoveRequested, true);
	return BTStatus::Success;
}


BTStatus EndTurnTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst; 
	std::cout << "[AI][Task] EndTurn request\n";
	bb.Set(BlackboardKeys::EndTurnRequested, true);
	return BTStatus::Success;
}