#include "UIPass.h"

void UIPass::Execute(const RenderData::FrameData& frame)
{
    const auto& context = frame.context;

    XMMATRIX tm = XMMatrixIdentity();
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, tm);
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mView, tm);

    XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 1.0f, 100.0f);

    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mProj, mProj);
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mVP, mProj);


    //현재는 depthpass에서 먼저 그려주기 때문에 여기서 지워버리면 안된다. 지울 위치를 잘 찾아보자
    //ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);
    m_RenderContext.pDXDC->OMSetRenderTargets(1, m_RenderContext.pRTView.GetAddressOf(), m_RenderContext.pDSViewScene_Depth.Get());
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());

    for (size_t index : GetQueue())
    {
        for (const auto& [layer, items] : frame.renderItems)
        {

            const auto& item = items[index];

            m_RenderContext.BCBuffer.mWorld = item.world;

            UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));


            const auto* vertexBuffers = m_RenderContext.vertexBuffers;
            const auto* indexBuffers = m_RenderContext.indexBuffers;
            const auto* indexcounts = m_RenderContext.indexCounts;

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
                    UINT32 indexCount = countIt->second;
                    if (vb && ib)
                    {
                        const UINT stride = sizeof(RenderData::Vertex);
                        const UINT offset = 0;

                        //★여기 상태 설정하는것도 어떻게 할 지 정해진 뒤에 수정을 해야함

                        m_RenderContext.pDXDC->OMSetBlendState(m_RenderContext.BState[BS::DEFAULT].Get(), NULL, 0xFFFFFFFF);
                        m_RenderContext.pDXDC->RSSetState(m_RenderContext.RState[RS::SOLID].Get());         //안전을 위해 SOLID
                        m_RenderContext.pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[DS::DEPTH_OFF].Get(), 0);
                        m_RenderContext.pDXDC->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
                        m_RenderContext.pDXDC->IASetInputLayout(m_RenderContext.inputLayout.Get());
                        m_RenderContext.pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                        m_RenderContext.pDXDC->VSSetShader(m_RenderContext.VS.Get(), nullptr, 0);
                        m_RenderContext.pDXDC->PSSetShader(m_RenderContext.PS.Get(), nullptr, 0);
                        m_RenderContext.pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
                        m_RenderContext.pDXDC->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
                        m_RenderContext.pDXDC->DrawIndexed(indexCount, 0, 0);

                    }
                }
            }
        }
    }

}