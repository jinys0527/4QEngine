#include "BlurPass.h"

void BlurPass::Execute(const RenderData::FrameData& frame)
{
    if (m_RenderContext.isEditCam)
        return;
    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();

#pragma region Init
    FLOAT backcolor[4] = { 0.21f, 0.21f, 0.21f, 1.0f };
    SetRenderTarget(m_RenderContext.pRTView_Blur.Get(), nullptr, backcolor);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, dxdc);
    SetBlendState(BS::DEFAULT);
    SetRasterizerState(RS::SOLID);
    SetDepthStencilState(DS::DEPTH_OFF);
    SetSamplerState();

#pragma endregion


    XMMATRIX tm = XMMatrixIdentity();
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, tm);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, tm);

    XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 1.0f, 100.0f);

    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, mProj);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, mProj);
    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));

    dxdc->PSSetShaderResources(0, 1, m_RenderContext.pTexRvScene_Refraction.GetAddressOf());

    dxdc->VSSetShader(m_RenderContext.VS_FSTriangle.Get(), nullptr, 0);
    dxdc->PSSetShader(m_RenderContext.PS_Quad.Get(), nullptr, 0);

    m_RenderContext.DrawFSTriangle();

    dxdc->GenerateMips(m_RenderContext.pTexRvScene_Blur.Get());
}
