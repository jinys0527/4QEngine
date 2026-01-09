#include "RenderPass.h"

void RenderPass::Setup(const RenderData::FrameData& frame)
{
	BuildQueue(frame);
}

bool RenderPass::ShouldIncludeRenderItem(const RenderData::RenderItem& /*item*/) const
{
	return true;
}

void RenderPass::BuildQueue(const RenderData::FrameData& frame)
{
	m_Queue.clear();
	m_Queue.reserve(frame.renderItems.size());

	for (size_t index = 0; index < frame.renderItems.size(); ++index)
	{
		for (const auto& [layer, items] : frame.renderItems)
		{
			if (ShouldIncludeRenderItem(items[index]))
			{
				m_Queue.push_back(index);
			}
		}
	}
}
