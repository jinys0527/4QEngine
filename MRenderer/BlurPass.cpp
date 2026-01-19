#include "BlurPass.h"

void BlurPass::Execute(const RenderData::FrameData& frame)
{
	const auto& context = frame.context;

    SetRenderTarget(m_RenderContext.pRTView_Blur.Get(), nullptr);

    XMMATRIX tm = XMMatrixIdentity();
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, tm);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, tm);

    XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 1.0f, 100.0f);

    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, mProj);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, mProj);
    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));

    SetRenderTarget(m_RenderContext.pRTView_Blur.Get(), nullptr);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());      //가로세로 절반

    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();

    dxdc->PSSetShaderResources(0, 1, m_RenderContext.pTexRvScene_Imgui.GetAddressOf());

    dxdc->VSSetShader(m_RenderContext.VS_Quad.Get(), nullptr, 0);
    dxdc->PSSetShader(m_RenderContext.PS_Quad.Get(), nullptr, 0);

    SetDepthStencilState(DS::DEPTH_OFF);
    m_RenderContext.DrawFullscreenQuad();

    dxdc->GenerateMips(m_RenderContext.pTexRvScene_Blur.Get());
}
