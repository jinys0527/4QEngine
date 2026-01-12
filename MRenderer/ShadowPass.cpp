#include "ShadowPass.h"

void ShadowPass::Execute(const RenderData::FrameData& frame)
{
    const auto& context = frame.context;
    if (frame.lights.empty())
        return;
    const auto& mainlight = frame.lights[0];


    //0번이 전역광이라고 가정...
    XMMATRIX lightview, lightproj;
    XMVECTOR maincampos = XMLoadFloat3(&context.gameCamera.cameraPos); //원래는 주인공 위치가 더 좋은데, 일단 카메라 위치로 해도 크게 상관 없을 듯
    XMVECTOR dir = XMLoadFloat3(&mainlight.diretion);
    XMVECTOR pos = dir * -10.f + maincampos;
    XMVECTOR look = maincampos;
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

     lightview = XMMatrixLookAtLH(pos, look, up);
    //원근 투영
    //lightProj = XMMatrixPerspectiveFovLH(XMConvertToRadians(15), 1.0f, 0.1f, 1000.f);
    //직교 투영
     lightproj = XMMatrixOrthographicLH(64, 64, 0.1f, 200.f);

     //텍스처 좌표 변환
     XMFLOAT4X4 m = {
         0.5f,  0.0f, 0.0f, 0.0f,
         0.0f, -0.5f, 0.0f, 0.0f,
         0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, 0.0f, 1.0f
     };

     XMMATRIX mscale = XMLoadFloat4x4(&m);
     XMMATRIX mLightTM;
     mLightTM = lightview * lightproj * mscale;

     XMStoreFloat4x4(&m_RenderContext.BCBuffer.mView, lightview);
     XMStoreFloat4x4(&m_RenderContext.BCBuffer.mProj, lightproj);
     XMStoreFloat4x4(&m_RenderContext.BCBuffer.mVP, lightview * lightproj);


     //★ 나중에 그림자 매핑용 행렬 위치 정해지면 상수 버퍼 set
     //XMStoreFloat4x4();

     m_RenderContext.pDXDC->OMSetRenderTargets(0, nullptr, m_RenderContext.pDSViewScene_Shadow.Get());
     m_RenderContext.pDXDC->ClearDepthStencilView(m_RenderContext.pDSViewScene_Shadow.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
     SetViewPort(m_RenderContext.ShadowTextureSize.width, m_RenderContext.ShadowTextureSize.height, m_RenderContext.pDXDC.Get());
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

                        //ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);       //클리어만 해도 바인딩이 됨

                        //m_RenderContext.pDXDC->OMSetBlendState(m_RenderContext.BState[BS::DEFAULT].Get(), nullptr, 0xFFFFFFFF);
                        m_RenderContext.pDXDC->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
                        m_RenderContext.pDXDC->RSSetState(m_RenderContext.RState[RS::CULLBACK].Get());
                        m_RenderContext.pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[DS::DEPTH_ON].Get(), 0);
                        m_RenderContext.pDXDC->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
                        m_RenderContext.pDXDC->IASetInputLayout(m_RenderContext.inputLayout.Get());
                        m_RenderContext.pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                        m_RenderContext.pDXDC->VSSetShader(m_RenderContext.VS.Get(), nullptr, 0);
                        //m_RenderContext.pDXDC->PSSetShader(m_RenderContext.PS.Get(), nullptr, 0);
                        m_RenderContext.pDXDC->PSSetShader(nullptr, nullptr, 0);
                        m_RenderContext.pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
                        m_RenderContext.pDXDC->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
                        m_RenderContext.pDXDC->DrawIndexed(icount, 0, 0);
                    }
                }
            }
        }
    }
}
