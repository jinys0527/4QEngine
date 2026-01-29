#pragma once

#include "Task.h"

class UpdateTargetLocationTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class MoveToTargetTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class ApproachTargetTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class MeleeAttackTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class RangedAttackTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class SelectRunOffTargetTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class RunOffMoveTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class MaintainRangeTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class PatrolMoveTask : public Task 
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class EndTurnTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

