#pragma once
#include "RenderPass.h"

// Input: 
// - FrameData: 불투명한 오브젝트에 대한 FrameData(mworld, skindata, light, ...) view, proj는 light를 통해 패스에서 계산
// - ShaderResourceView: None
// - Depthview: None
// - Viewport: 16384, 16384
// - BlendState: DEFAULT or nullptr,        현재는 nullptr
// - RasterizeState: CULLBACK
// - DepthStencilState: Depth ON, StencilOFF
// - InputLayout: pos
// output:
// - RenderTarget: None
// - Depthview: m_RenderContext.pDSViewScene_Shadow
//
class ShadowPass : public RenderPass
{
public:
	ShadowPass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Opaque"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(const RenderData::RenderItem& item) const override
    {
        
        return false;
    }

};

