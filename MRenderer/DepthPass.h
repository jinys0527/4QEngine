#pragma once
#include "RenderPass.h"

class DepthPass : public RenderPass
{
public:
    DepthPass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Opaque"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(const RenderData::RenderItem& item) const override
    {

        return true;
    }

};

