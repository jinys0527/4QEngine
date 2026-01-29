#include "CombatBehaviorTreeFactory.h"
#include "Selector.h"
#include "Sequance.h"
#include "BlackboardConditionTask.h"
#include "CombatBehaviorTasks.h"
#include "CombatBehaviorServices.h"
#include "BlackboardKeys.h"
#include "Decorator.h"
#include "EventDispatcher.h"

namespace CombatBehaviorTreeFactory
{

	std::shared_ptr<Node> BuildDefaultTree(EventDispatcher* dispatcher)
	{
		auto root = std::make_shared<Selector>();

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 도주
		{
			auto runOffSequence = std::make_shared<Sequance>();
			runOffSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::ShouldRunOff, true));
			runOffSequence->AddChild(std::make_shared<SelectRunOffTargetTask>());
			runOffSequence->AddChild(std::make_shared<RunOffMoveTask>());
			runOffSequence->AddChild(std::make_shared<EndTurnTask>());
			root->AddChild(runOffSequence);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 원거리 공격
		{
			auto rangedSequence = std::make_shared<Sequance>();
			rangedSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::HasTarget, true));
			rangedSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::PreferRanged, true));
			rangedSequence->AddChild(std::make_shared<MaintainRangeTask>());
			rangedSequence->AddChild(std::make_shared<RangedAttackTask>());
			rangedSequence->AddChild(std::make_shared<EndTurnTask>());
			root->AddChild(rangedSequence);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 근접 공격
		{
			auto meleeSequence = std::make_shared<Sequance>();
			meleeSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::HasTarget, true));
			meleeSequence->AddChild(std::make_shared<ApproachTargetTask>());
			meleeSequence->AddChild(std::make_shared<MeleeAttackTask>());
			meleeSequence->AddChild(std::make_shared<EndTurnTask>());
			root->AddChild(meleeSequence);
		}


		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 탐색 서비스
		{
			auto searchSequence = std::make_shared<Sequance>();
			auto updateTask = std::make_shared<UpdateTargetLocationTask>();
			updateTask->AddService(std::make_unique<TargetSenseService>());
			updateTask->AddService(std::make_unique<CombatStateSyncService>());
			updateTask->AddService(std::make_unique<RangeUpdateService>());
			updateTask->AddService(std::make_unique<EstimatePlayerDamageService>());
			updateTask->AddService(std::make_unique<RepathService>());
			updateTask->AddService(std::make_unique<AIRequestDispatchService>(dispatcher));
			searchSequence->AddChild(updateTask);
			searchSequence->AddChild(std::make_shared<EndTurnTask>());
			root->AddChild(searchSequence);
		}

		return root;
	}
}
