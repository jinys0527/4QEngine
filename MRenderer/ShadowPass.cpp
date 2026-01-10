#include "ShadowPass.h"

void ShadowPass::Execute(const RenderData::FrameData& frame)
{
    const auto& context = frame.context;

    m_RenderContext.BCBuffer.mView = context.view;
    m_RenderContext.BCBuffer.mProj = context.proj;
    m_RenderContext.BCBuffer.mVP = context.viewProj;

    bool isVPUpdated = false;

    frame.lights[0].posiiton

    for (size_t index : GetQueue())
    {

        for (const auto& [layer, items] : frame.renderItems)
        {

            const auto& item = items[index];

            m_RenderContext.BCBuffer.mWorld = item.world;

            UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));


            const auto* vertexBuffers = m_RenderContext.vertexBuffers;
            const auto* indexBuffers = m_RenderContext.indexBuffers;
            const auto* indexcounts = m_RenderContext.indexcounts;

            if (vertexBuffers && indexBuffers && indexcounts && item.mesh.IsValid())
            {
                const UINT bufferIndex = item.mesh.id;
                const auto vbIt = vertexBuffers->find(bufferIndex);
                const auto ibIt = indexBuffers->find(bufferIndex);
                const auto countIt = indexcounts->find(bufferIndex);
                if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexcounts->end())
                {
                    ID3D11Buffer* vb = vbIt->second.Get();
                    ID3D11Buffer* ib = ibIt->second.Get();
                    UINT32 icount = countIt->second;
                    if (vb && ib)
                    {
                        const UINT stride = sizeof(RenderData::Vertex);
                        const UINT offset = 0;

                        ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);

                        m_RenderContext.pDXDC->OMSetBlendState(m_RenderContext.BState[BS::DEFAULT].Get(), NULL, 0xFFFFFFFF);
                        m_RenderContext.pDXDC->RSSetState(m_RenderContext.RState[RS::CULLBACK].Get());
                        m_RenderContext.pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[DS::DEPTH_ON].Get(), 0);
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
        }
    }

}
