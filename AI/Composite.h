#pragma once
#include "Node.h"
#include <vector>
#include <memory>

class Composite : public Node
{
public:
	void AddChild(std::shared_ptr<Node> node)
	{
		m_Children.push_back(node);
	}

protected:
	std::vector<std::shared_ptr<Node>> m_Children;
};

