#pragma once
#include "RenderPass.h"

class ShadowPass : public RenderPass
{
public:
	ShadowPass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Opaque"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(const RenderData::RenderItem& item) const override
    {
        
        return true;
    }

};

