#pragma once
#include "Composite.h"

class Sequance : public Composite
{
public:
	void AddChild(std::shared_ptr<Node> child) { m_Children.push_back(std::move(child)); }

	BTStatus Tick(BTInstance& inst, Blackboard& bb) override;
	void     OnAbort(BTInstance& inst, Blackboard& bb) override;

	const std::vector<std::shared_ptr<Node>>& GetChildren() const { return m_Children; }

private:
	std::vector<std::shared_ptr<Node>> m_Children;
};

