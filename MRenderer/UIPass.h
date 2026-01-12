#pragma once
#include "RenderPass.h"

// Input: 
// - FrameData: UI object           
// - ShaderResourceView: None
// - Depthview: None
// - Viewport: ScreenSize
// - BlendState: DEFAULT or nullptr             //현재는 DEFAULT
// - RasterizeState: SOLID                      //안전을 위해
// - DepthStencilState: Depth OFF, StencilOFF
// - InputLayout: pos, uv                       //적용 안됨
// output:
// - RenderTarget: m_RenderContext.pRTView (main RT)
// - Depthview: None
//


class UIPass : public RenderPass
{
public:
    UIPass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Opaque"; }

    void Execute(const RenderData::FrameData& frame) override;
protected:
    bool ShouldIncludeRenderItem(const RenderData::RenderItem& item) const override
    {
        // 예: 투명/블렌딩 제외 같은 조건
        return true;
    }
};
