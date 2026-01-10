#include "ResourceStore.h"
#include "OpaquePass.h"

#include <algorithm>
#include <iostream>

void OpaquePass::Execute(const RenderData::FrameData& frame)
{
    const auto& context = frame.context;

    m_RenderContext.BCBuffer.mView = context.view;
    m_RenderContext.BCBuffer.mProj = context.proj;
    m_RenderContext.BCBuffer.mVP = context.viewProj;

    

    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));


    for (size_t index : GetQueue())
    {
        for (const auto& [layer, items] : frame.renderItems)
        {

            const auto& item = items[index];

            m_RenderContext.BCBuffer.mWorld = item.world;

            if (m_RenderContext.pSkinCB && item.skinningPaletteCount > 0)
            {
                const size_t paletteStart = item.skinningPaletteOffset;
                const size_t paletteCount = item.skinningPaletteCount;
                const size_t paletteSize  = frame.skinningPalettes.size();
                const size_t maxCount     = min(static_cast<size_t>(kMaxSkinningBones), paletteCount);
                // Clamp
                const size_t safeCount    = (paletteStart + maxCount <= paletteSize) ? maxCount : (paletteSize > paletteStart ? paletteSize - paletteStart : 0);
                
                m_RenderContext.SkinCBuffer.boneCount = static_cast<UINT>(safeCount);
                for (size_t i = 0; i < safeCount; ++i)
                {
                    m_RenderContext.SkinCBuffer.bones[i] = frame.skinningPalettes[paletteStart + i];
                }
                UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pSkinCB.Get(), &m_RenderContext.SkinCBuffer, sizeof(SkinningConstBuffer));
                
				m_RenderContext.pDXDC->VSSetConstantBuffers(1, 1, m_RenderContext.pBCB.GetAddressOf());
            }
			else if (m_RenderContext.pSkinCB)
			{
				m_RenderContext.SkinCBuffer.boneCount = 0;
				UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pSkinCB.Get(), &m_RenderContext.SkinCBuffer, sizeof(SkinningConstBuffer));
                m_RenderContext.pDXDC->VSSetConstantBuffers(1, 1, m_RenderContext.pSkinCB.GetAddressOf());
			}


            const auto* vertexBuffers = m_RenderContext.vertexBuffers;
            const auto* indexBuffers = m_RenderContext.indexBuffers;
            const auto* indexcounts = m_RenderContext.indexcounts;

            if (vertexBuffers && indexBuffers && item.mesh.IsValid())
            {
                const UINT bufferIndex = item.mesh.id;
                if (bufferIndex < vertexBuffers->size() && bufferIndex < indexBuffers->size())
                {
                    ID3D11Buffer* vb = (*vertexBuffers)[bufferIndex].Get();
                    ID3D11Buffer* ib = (*indexBuffers)[bufferIndex].Get();
                    UINT32 icount = (*indexcounts)[bufferIndex];
                    if (vb && ib)
                    {
                        const UINT stride = sizeof(RenderData::Vertex);
                        const UINT offset = 0;

                        ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);

                        m_RenderContext.pDXDC->RSSetState(m_RenderContext.RState[RS::SOLID].Get());
                        m_RenderContext.pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[DS::DEPTH_OFF].Get(), 0);
                        m_RenderContext.pDXDC->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
                        m_RenderContext.pDXDC->IASetInputLayout(m_RenderContext.inputLayout.Get());
                        m_RenderContext.pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                        m_RenderContext.pDXDC->VSSetShader(m_RenderContext.VS.Get(), nullptr, 0);
                        m_RenderContext.pDXDC->PSSetShader(m_RenderContext.PS.Get(), nullptr, 0);
                        m_RenderContext.pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
                        m_RenderContext.pDXDC->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
                        m_RenderContext.pDXDC->DrawIndexed(icount, 0, 0);

                    }
                }
            }







            //아래 세팅하는건 함수로 빼두자
            //UpdateDynamicBuffer(g_pDXDC.Get(), m_RenderContext.pBCB.Get(), &m_RenderContext.BCBuffer, sizeof(BaseConstBuffer));
            //g_pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());

        }
    }
}
