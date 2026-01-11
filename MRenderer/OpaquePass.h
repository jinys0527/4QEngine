#pragma once
#include "RenderPass.h"

// 불투명한 물체 
class OpaquePass : public RenderPass
{
public:
    OpaquePass(RenderContext& context, AssetLoader& assetloader) : RenderPass(context, assetloader) {}
    std::string_view GetName() const override { return "Opaque"; }

    void Execute(const RenderData::FrameData& frame) override;
    void DrawMesh(
        ID3D11DeviceContext* dc,
        ID3D11Buffer* vb,
        ID3D11Buffer* ib,
        UINT indexCount
    );
protected:
    bool ShouldIncludeRenderItem(const RenderData::RenderItem& item) const override
    {
        // 예: 투명/블렌딩 제외 같은 조건
        return true;
    }
};
