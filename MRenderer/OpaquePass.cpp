#include "ResourceStore.h"
#include "OpaquePass.h"

#include <algorithm>
#include <iostream>

void OpaquePass::Execute(const RenderData::FrameData& frame)
{
    const auto& context = frame.context;

    if (!m_RenderContext.isEditCam)
    {
        m_RenderContext.BCBuffer.mView = context.gameCamera.view;
        m_RenderContext.BCBuffer.mProj = context.gameCamera.proj;
        m_RenderContext.BCBuffer.mVP = context.gameCamera.viewProj;

        m_RenderContext.pDXDC->OMSetRenderTargets(1, m_RenderContext.pRTView_Imgui.GetAddressOf(), m_RenderContext.pDSViewScene_Depth.Get());

    }
    else if (m_RenderContext.isEditCam)
    {
        m_RenderContext.BCBuffer.mView = context.editorCamera.view;
        m_RenderContext.BCBuffer.mProj = context.editorCamera.proj;
        m_RenderContext.BCBuffer.mVP = context.editorCamera.viewProj;

        m_RenderContext.pDXDC->OMSetRenderTargets(1, m_RenderContext.pRTView_Imgui_edit.GetAddressOf(), m_RenderContext.pDSViewScene_Depth.Get());

    }

    //빛 상수 버퍼 set
    if (!frame.lights.empty())
    {
        const auto& light = frame.lights[0];
        Light dirlight;
        dirlight.vDir = light.diretion;
        dirlight.Color = light.color;
        dirlight.Intensity = light.intensity;
        dirlight.mLightViewProj = light.lightViewProj;
        dirlight.CastShadow = light.castShadow;

        m_RenderContext.LightCBuffer.lights[0] = dirlight;

        UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pLightCB.Get(), &m_RenderContext.LightCBuffer, sizeof(BaseConstBuffer));

    }



    //★이부분 에디터랑 게임 씬 크기가 다르면 이것도 if문안에 넣어야할듯
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());


    
    //현재는 depthpass에서 먼저 그려주기 때문에 여기서 지워버리면 안된다. 지울 위치를 잘 찾아보자
    //ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);

    for (size_t index : GetQueue())
    {
        for (const auto& [layer, items] : frame.renderItems)
        {
            for (const auto& item : items)
            {
				XMMATRIX mtm = XMMatrixIdentity();
				mtm = XMLoadFloat4x4(&item.world);

				XMFLOAT4X4 tm;
				XMStoreFloat4x4(&tm, mtm);

				m_RenderContext.BCBuffer.mWorld = tm;

				UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &m_RenderContext.BCBuffer, sizeof(BaseConstBuffer));

				if (m_RenderContext.pSkinCB && item.skinningPaletteCount > 0)
				{
					const size_t paletteStart = item.skinningPaletteOffset;
					const size_t paletteCount = item.skinningPaletteCount;
					const size_t paletteSize = frame.skinningPalettes.size();
					const size_t maxCount = min(static_cast<size_t>(kMaxSkinningBones), paletteCount);
					// Clamp
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
				const auto* indexCounts = m_RenderContext.indexCounts;


				BOOL castshadow = frame.lights[index].castShadow;

				if (vertexBuffers && indexBuffers && indexCounts && item.mesh.IsValid())
				{
					const UINT bufferIndex = item.mesh.id;
					const auto vbIt = vertexBuffers->find(bufferIndex);
					const auto ibIt = indexBuffers->find(bufferIndex);
					const auto countIt = indexCounts->find(bufferIndex);

					if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexCounts->end())
					{
						ID3D11Buffer* vb = vbIt->second.Get();
						ID3D11Buffer* ib = ibIt->second.Get();
						const UINT32 fullCount = countIt->second;
						const bool useSubMesh = item.useSubMesh;
						const UINT32 indexCount = useSubMesh ? item.indexCount : fullCount;
						const UINT32 indexStart = useSubMesh ? item.indexStart : 0;
						DrawMesh(m_RenderContext.pDXDC.Get(), vb, ib, useSubMesh, indexCount, indexStart, castshadow);
					}
				}
            }
        }
    }
}

void OpaquePass::DrawMesh(
    ID3D11DeviceContext* dc,
    ID3D11Buffer* vb,
    ID3D11Buffer* ib,
    BOOL useSubMesh,
    UINT indexCount,
    UINT indexStart,
    BOOL castshadow
)
{
    const UINT stride = sizeof(RenderData::Vertex);
    const UINT offset = 0;

    dc->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    dc->RSSetState(m_RenderContext.RState[RS::CULLBACK].Get());
    dc->OMSetDepthStencilState(m_RenderContext.DSState[DS::DEPTH_ON].Get(), 0);

    dc->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    dc->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    dc->IASetInputLayout(m_RenderContext.inputLayout.Get());
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    dc->VSSetShader(m_RenderContext.VS.Get(), nullptr, 0);
    dc->PSSetShader(m_RenderContext.PS.Get(), nullptr, 0);

    dc->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
    dc->PSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
    dc->VSSetConstantBuffers(1, 1, m_RenderContext.pLightCB.GetAddressOf());
    dc->PSSetConstantBuffers(1, 1, m_RenderContext.pLightCB.GetAddressOf());

    //그림자가 드리워진다면 다른 픽셀쉐이더를 실행하게 한다?
    //SetShaderResource(dc);
    //SetSamplerState(dc);

	if (useSubMesh)
	{
        dc->DrawIndexed(indexCount, indexStart, 0);
	}
	else
	{
        dc->DrawIndexed(indexCount, 0, 0);
	}
}