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
		auto buildSenseUpdateTask = [dispatcher]()
			{
				auto updateTask = std::make_shared<UpdateTargetLocationTask>();
				updateTask->AddService(std::make_unique<TargetSenseService>());
				updateTask->AddService(std::make_unique<CombatStateSyncService>());
				updateTask->AddService(std::make_unique<RangeUpdateService>());
				updateTask->AddService(std::make_unique<EstimatePlayerDamageService>());
				updateTask->AddService(std::make_unique<RepathService>());
				updateTask->AddService(std::make_unique<AIRequestDispatchService>(dispatcher));
				return updateTask;
			};

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 전투 여부 분기 (A 구조)
		{
			auto combatSequence = std::make_shared<Sequance>();
			combatSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::HasTarget, true));
			combatSequence->AddChild(buildSenseUpdateTask());
			auto combatSelector = std::make_shared<Selector>();

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 도주
			{
				auto runOffSequence = std::make_shared<Sequance>();
				runOffSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::ShouldRunOff, true));
				runOffSequence->AddChild(std::make_shared<SelectRunOffTargetTask>());
				runOffSequence->AddChild(std::make_shared<RunOffMoveTask>());
				runOffSequence->AddChild(std::make_shared<EndTurnTask>());
				combatSelector->AddChild(runOffSequence);
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
				combatSelector->AddChild(rangedSequence);
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 근접 공격
			{
				auto meleeSequence = std::make_shared<Sequance>();
				meleeSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::HasTarget, true));
				meleeSequence->AddChild(std::make_shared<ApproachTargetTask>());
				meleeSequence->AddChild(std::make_shared<MeleeAttackTask>());
				meleeSequence->AddChild(std::make_shared<EndTurnTask>());
				combatSelector->AddChild(meleeSequence);
			}

			combatSequence->AddChild(combatSelector);
			root->AddChild(combatSequence);
		}
				
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 비전투 탐색 (정찰/이동/대기 역할)
		{
			auto nonCombatSequence = std::make_shared<Sequance>();
			nonCombatSequence->AddChild(buildSenseUpdateTask());
			nonCombatSequence->AddChild(std::make_shared<PatrolMoveTask>());
			nonCombatSequence->AddChild(std::make_shared<EndTurnTask>());
			root->AddChild(nonCombatSequence);
		}

		return root;
	}
}
