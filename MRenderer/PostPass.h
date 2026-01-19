#pragma once
#include "RenderPass.h"

// Input: 
// - FrameData: None            //적용 안됨 PostPass.cpp의 ★을 참고
// - ShaderResourceView: None
// - Depthview: None
// - Viewport: ScreenSize
// - BlendState: ADD
// - RasterizeState: CULLBACK
// - DepthStencilState: Depth OFF, StencilOFF
// - InputLayout: pos, uv           //적용 안됨
// output:
// - RenderTarget: m_RenderContext.pRTView (main RT)
// - Depthview: None
//

class PostPass : public RenderPass
{
public:
    PostPass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Opaque"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(RenderData::RenderLayer layer, const RenderData::RenderItem& item) const override
    {
        // 예: 투명/블렌딩 제외 같은 조건
        return false;
    }
};
