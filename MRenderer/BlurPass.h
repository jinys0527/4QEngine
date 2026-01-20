#pragma once
#include "RenderPass.h"

// 모든 물체를 그린 후 블러를 그리기 위한 렌더 패스
// Input: 
// - FrameData: None
// - ShaderResourceView: m_RenderContext.pTexRvScene_Imgui
// - Depthview: None
// - Viewport: ScreenSize / 2
// - BlendState: DEFAULT or nullptr,        
// - RasterizeState: CULLBACK or SOLID		안전을 위한 SOLID
// - DepthStencilState: Depth OFF, StencilOFF
// - InputLayout: pos, nrm, uv, Tangent
// output:
// - RenderTarget: m_RenderContext.pRTView (main RT)
// - Depthview: m_RenderContext.pDSViewScene_Depth (Updated from Opaque DepthPass)
//


class BlurPass : public RenderPass
{
public:
    BlurPass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Opaque"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(RenderData::RenderLayer layer, const RenderData::RenderItem& item) const override
    {
        // 예: 투명/블렌딩 제외 같은 조건
        return true;
    }

};

