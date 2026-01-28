#pragma once

#include <memory>

class Node;

namespace CombatBehaviorTreeFactory
{
	std::shared_ptr<Node> BuildDefaultTree();
}