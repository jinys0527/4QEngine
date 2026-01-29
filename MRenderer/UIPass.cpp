#include "UIPass.h"

void UIPass::Execute(const RenderData::FrameData & frame)
{
    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
    ID3D11ShaderResourceView* nullSRVs[128] = { nullptr };
    dxdc->PSSetShaderResources(0, 128, nullSRVs);
    SetRenderTarget(m_RenderContext.pRTView_Post.Get(), nullptr);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());
    SetBlendState(BS::ALPHABLEND);
    SetRasterizerState(RS::SOLID);
    SetDepthStencilState(DS::DEPTH_OFF);
    SetSamplerState();
#pragma endregion

    const auto& context = frame.context;

    XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 1.0f, 100.0f);

    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, mProj);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, mProj);
    UpdateDynamicBuffer(dxdc, m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));

    for (const auto& queueItem : GetQueue())
    {
        const auto& uiitem = *queueItem.uielement;

        XMMATRIX mPos = XMMatrixTranslation(uiitem.position.x, uiitem.position.y, 0);
        XMMATRIX mRot = XMMatrixRotationZ(uiitem.rotation);
        XMMATRIX mScale = XMMatrixScaling(uiitem.size.x, uiitem.size.y, 0);

        XMMATRIX mTm = mScale * mRot * mPos;

        XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, mTm);
        UpdateDynamicBuffer(dxdc, m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(BaseConstBuffer));

        m_RenderContext.UIBuffer.Color = uiitem.color;
        m_RenderContext.UIBuffer.Opacity = uiitem.opacity;

        const auto& texitem = *queueItem.uitextelement;

        mPos = XMMatrixTranslation(texitem.position.x, texitem.position.y, 0);
        XMStoreFloat4x4(&m_RenderContext.UIBuffer.TextPosition, mPos);

        m_RenderContext.UIBuffer.TextColor = texitem.color;

        UpdateDynamicBuffer(dxdc, m_RenderContext.pUIB.Get(), &(m_RenderContext.UIBuffer), sizeof(UIBuffer));








		const auto& item = *queueItem.item;

		const auto* textures = m_RenderContext.textures;
		const auto* vertexShaders = m_RenderContext.vertexShaders;
		const auto* pixelShaders = m_RenderContext.pixelShaders;

		ID3D11VertexShader* vertexShader = m_RenderContext.VS_PBR.Get();
		ID3D11PixelShader* pixelShader = m_RenderContext.PS_PBR.Get();


		const RenderData::MaterialData* mat = nullptr;
		if (item.useMaterialOverrides)
		{
			mat = &item.materialOverrides;
		}
		else if (item.material.IsValid())
		{
			mat = m_AssetLoader.GetMaterials().Get(item.material);
		}
		if (mat)
		{
			m_RenderContext.MatBuffer.baseColor = mat->baseColor;
			m_RenderContext.MatBuffer.saturation = mat->saturation;
			m_RenderContext.MatBuffer.lightness = mat->lightness;
			UpdateDynamicBuffer(dxdc, m_RenderContext.pMatB.Get(), &m_RenderContext.MatBuffer, sizeof(MaterialBuffer));
			dxdc->VSSetConstantBuffers(5, 1, m_RenderContext.pMatB.GetAddressOf());
			dxdc->PSSetConstantBuffers(5, 1, m_RenderContext.pMatB.GetAddressOf());
		}
		if (textures && mat)
		{
			if (mat->shaderAsset.IsValid())
			{
				const auto* shaderAsset = m_AssetLoader.GetShaderAssets().Get(mat->shaderAsset);
				if (shaderAsset)
				{
					if (vertexShaders && shaderAsset->vertexShader.IsValid())
					{
						const auto shaderIt = vertexShaders->find(shaderAsset->vertexShader);
						if (shaderIt != vertexShaders->end() && shaderIt->second.vertexShader)
						{
							vertexShader = shaderIt->second.vertexShader.Get();
						}
					}

					if (pixelShaders && shaderAsset->pixelShader.IsValid())
					{
						const auto shaderIt = pixelShaders->find(shaderAsset->pixelShader);
						if (shaderIt != pixelShaders->end() && shaderIt->second.pixelShader)
						{
							pixelShader = shaderIt->second.pixelShader.Get();
						}
					}
				}
			}

			if (vertexShaders && mat->vertexShader.IsValid())
			{
				const auto shaderIt = vertexShaders->find(mat->vertexShader);
				if (shaderIt != vertexShaders->end() && shaderIt->second.vertexShader)
				{
					vertexShader = shaderIt->second.vertexShader.Get();
				}
			}

			if (pixelShaders && mat->pixelShader.IsValid())
			{
				const auto shaderIt = pixelShaders->find(mat->pixelShader);
				if (shaderIt != pixelShaders->end() && shaderIt->second.pixelShader)
				{
					pixelShader = shaderIt->second.pixelShader.Get();
				}
			}



#pragma region TextureBinding
			for (UINT slot = 0; slot < static_cast<UINT>(RenderData::MaterialTextureSlot::TEX_MAX); ++slot)
			{
				const TextureHandle h = mat->textures[slot];
				if (!h.IsValid())
					continue;

				const auto tIt = textures->find(h);
				if (tIt == textures->end())
					continue;

				ID3D11ShaderResourceView* srv = tIt->second.Get();


				m_RenderContext.pDXDC->PSSetShaderResources(11 + slot, 1, &srv);
			}
		}

    }
}
