#include "PostPass.h"



void PostPass::Execute(const RenderData::FrameData& frame)
{
    SetRenderTarget(m_RenderContext.pRTView_Post.Get(), nullptr);
    //먼저 화면전체 QUad그리기
    const auto& context = frame.context;

    XMMATRIX tm = XMMatrixIdentity();
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, tm);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, tm);

    XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 1.0f, 100.0f);

    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, mProj);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, mProj);
    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));


    //현재는 depthpass에서 먼저 그려주기 때문에 여기서 지워버리면 안된다. 지울 위치를 잘 찾아보자
    //ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());

    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();

    dxdc->VSSetShader(m_RenderContext.VS_Post.Get(), nullptr, 0);
    dxdc->PSSetShader(m_RenderContext.PS_Post.Get(), nullptr, 0);
    dxdc->PSSetShaderResources(0, 1, m_RenderContext.pTexRvScene_Imgui.GetAddressOf());
    dxdc->PSSetShaderResources(1, 1, m_RenderContext.pTexRvScene_Blur.GetAddressOf());
    dxdc->PSSetShaderResources(2, 1, m_RenderContext.Vignetting.GetAddressOf());

    m_RenderContext.DrawFullscreenQuad();

    //★아래 프레임데이터를 순회하면서 그리는게 필요없어 보이는데 어떻게 넘겨줄지 몰라서 일단 남김.
    //for (size_t index : GetQueue())
    //{
    //    for (const auto& [layer, items] : frame.renderItems)
    //    {

    //        const auto& item = items[index];

    //        m_RenderContext.BCBuffer.mWorld = item.world;

    //        UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));


    //        const auto* vertexBuffers = m_RenderContext.vertexBuffers;
    //        const auto* indexBuffers = m_RenderContext.indexBuffers;
    //        const auto* indexcounts = m_RenderContext.indexcounts;

    //        if (vertexBuffers && indexBuffers && indexcounts && item.mesh.IsValid())
    //        {
    //            const UINT bufferIndex = item.mesh.id;
    //            const auto vbIt = vertexBuffers->find(bufferIndex);
    //            const auto ibIt = indexBuffers->find(bufferIndex);
    //            const auto countIt = indexcounts->find(bufferIndex);
    //            if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexcounts->end())
    //            {
    //                ID3D11Buffer* vb = vbIt->second.Get();
    //                ID3D11Buffer* ib = ibIt->second.Get();
    //                UINT32 icount = countIt->second;
    //                if (vb && ib)
    //                {
    //                    const UINT stride = sizeof(RenderData::Vertex);
    //                    const UINT offset = 0;

    //                    //★여기 상태 설정하는것도 어떻게 할 지 정해진 뒤에 수정을 해야함

    //                    m_RenderContext.pDXDC->OMSetBlendState(m_RenderContext.BState[BS::ADD].Get(), NULL, 0xFFFFFFFF);
    //                    m_RenderContext.pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[DS::DEPTH_OFF].Get(), 0);
    //                    m_RenderContext.pDXDC->RSSetState(m_RenderContext.RState[RS::CULLBACK].Get());
    //                    m_RenderContext.pDXDC->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    //                    m_RenderContext.pDXDC->IASetInputLayout(m_RenderContext.inputLayout.Get());
    //                    m_RenderContext.pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //                    m_RenderContext.pDXDC->VSSetShader(m_RenderContext.VS.Get(), nullptr, 0);
    //                    m_RenderContext.pDXDC->PSSetShader(m_RenderContext.PS.Get(), nullptr, 0);
    //                    m_RenderContext.pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
    //                    m_RenderContext.pDXDC->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    //                    m_RenderContext.pDXDC->DrawIndexed(icount, 0, 0);

    //                }
    //            }
    //        }
    //    }
    //}

}

