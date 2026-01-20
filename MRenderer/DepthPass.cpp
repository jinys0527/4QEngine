#include "DepthPass.h"

#include <algorithm>

void DepthPass::Execute(const RenderData::FrameData& frame)
{
    const auto& context = frame.context;
    if (frame.lights.empty())
        return;
    const auto& mainlight = frame.lights[0];

    m_RenderContext.CameraCBuffer.mView = context.gameCamera.view;
    m_RenderContext.CameraCBuffer.mProj = context.gameCamera.proj;
    m_RenderContext.CameraCBuffer.mVP = context.gameCamera.viewProj;
    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));


    m_RenderContext.pDXDC->OMSetRenderTargets(0, nullptr, m_RenderContext.pDSViewScene_Depth.Get());
    m_RenderContext.pDXDC->ClearDepthStencilView(m_RenderContext.pDSViewScene_Depth.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());

    SetDepthStencilState(DS::DEPTH_ON);

    for (const auto& queueItem : GetQueue())
    {
        const auto& item = *queueItem.item;
        SetBaseCB(item);

        if (m_RenderContext.pSkinCB && item.skinningPaletteCount > 0)
        {
            const size_t paletteStart = item.skinningPaletteOffset;
            const size_t paletteCount = item.skinningPaletteCount;
            const size_t paletteSize = frame.skinningPalettes.size();
            const size_t maxCount = min(static_cast<size_t>(kMaxSkinningBones), paletteCount);
            const size_t safeCount = (paletteStart + maxCount <= paletteSize) ? maxCount : (paletteSize > paletteStart ? paletteSize - paletteStart : 0);

            m_RenderContext.SkinCBuffer.boneCount = static_cast<UINT>(safeCount);
            for (size_t i = 0; i < safeCount; ++i)
            {
                m_RenderContext.SkinCBuffer.bones[i] = frame.skinningPalettes[paletteStart + i];
            }
            UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pSkinCB.Get(), &m_RenderContext.SkinCBuffer, sizeof(SkinningConstBuffer));

            m_RenderContext.pDXDC->VSSetConstantBuffers(1, 1, m_RenderContext.pSkinCB.GetAddressOf());
        }
        else if (m_RenderContext.pSkinCB)
        {
            m_RenderContext.SkinCBuffer.boneCount = 0;
            UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pSkinCB.Get(), &m_RenderContext.SkinCBuffer, sizeof(SkinningConstBuffer));
            m_RenderContext.pDXDC->VSSetConstantBuffers(1, 1, m_RenderContext.pSkinCB.GetAddressOf());
        }

        const auto* vertexBuffers = m_RenderContext.vertexBuffers;
        const auto* indexBuffers = m_RenderContext.indexBuffers;
        const auto* indexcounts = m_RenderContext.indexCounts;

        if (vertexBuffers && indexBuffers && indexcounts && item.mesh.IsValid())
        {
            const MeshHandle bufferHandle = item.mesh;
            const auto vbIt = vertexBuffers->find(bufferHandle);
            const auto ibIt = indexBuffers->find(bufferHandle);
            const auto countIt = indexcounts->find(bufferHandle);
            if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexcounts->end())
            {
                ID3D11Buffer* vb = vbIt->second.Get();
                ID3D11Buffer* ib = ibIt->second.Get();
                const bool useSubMesh = item.useSubMesh;
                const UINT32 indexCount = useSubMesh ? item.indexCount : countIt->second;
                const UINT32 indexStart = useSubMesh ? item.indexStart : 0;
                if (vb && ib)
                {
                    const UINT stride = sizeof(RenderData::Vertex);
                    const UINT offset = 0;

                    DrawMesh(vb, ib, m_RenderContext.VS_Shadow.Get(), nullptr, useSubMesh, indexCount, indexStart);
                }
            }
        }
    }
}
