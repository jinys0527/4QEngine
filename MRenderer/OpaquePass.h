#pragma once
#include "RenderPass.h"

// 불투명한 물체 
// Input: 
// - FrameData: 불투명한 오브젝트에 대한 FrameData(mworld, mview, mproj, skindata, light, ...)
// - ShaderResourceView: m_RenderContext.pDepthRV, m_RenderContext.pShadowRV
// - Depthview: m_RenderContext.pDSViewScene_Depth (from DepthPass)
// - Viewport: ScreenSize
// - BlendState: DEFAULT or nullptr,        현재는 nullptr
// - RasterizeState: CULLBACK
// - DepthStencilState: Depth ON, StencilOFF
// - InputLayout: pos, nrm, uv, Tangent
// output:
// - RenderTarget: m_RenderContext.pRTView (main RT)
// - Depthview: m_RenderContext.pDSViewScene_Depth (Updated from Opaque DepthPass)
//
class OpaquePass : public RenderPass
{
public:
    OpaquePass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Opaque"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(RenderData::RenderLayer layer, const RenderData::RenderItem& item) const override
    {
        // 예: 투명/블렌딩 제외 같은 조건
        return layer == RenderData::OpaqueItems;
    }
};
