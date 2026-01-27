#include "EmissivePass.h"

void EmissivePass::Execute(const RenderData::FrameData& frame)
{
    if (m_RenderContext.isEditCam)
        return;
    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
    FLOAT backcolor[4] = { 0.f, 0.f, 0.f, 1.0f };
    SetRenderTarget(, m_RenderContext.pDSViewScene_Depth.Get(), backcolor);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());
    SetBlendState(BS::ADD);
    SetRasterizerState(RS::CULLBACK);
    SetDepthStencilState(DS::DEPTH_OFF);
    SetSamplerState();

#pragma endregion

	SetCameraCB(frame);
	SetDirLight(frame);
	SetOtherLights(frame);

	for (const auto& queueItem : GetQueue())
	{
		const auto& item = *queueItem.item;
		SetBaseCB(item);
		SetMaskingTM(item, frame.context.gameCamera.cameraPos);

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

			m_RenderContext.pDXDC->VSSetConstantBuffers(3, 1, m_RenderContext.pSkinCB.GetAddressOf());
		}
		else if (m_RenderContext.pSkinCB)
		{
			m_RenderContext.SkinCBuffer.boneCount = 0;
			UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pSkinCB.Get(), &m_RenderContext.SkinCBuffer, sizeof(SkinningConstBuffer));
			m_RenderContext.pDXDC->VSSetConstantBuffers(3, 1, m_RenderContext.pSkinCB.GetAddressOf());
		}

		const auto* vertexBuffers = m_RenderContext.vertexBuffers;
		const auto* indexBuffers = m_RenderContext.indexBuffers;
		const auto* indexCounts = m_RenderContext.indexCounts;
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
			//머티리얼 버퍼 업데이트
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

#pragma endregion


		if (vertexBuffers && indexBuffers && indexCounts && item.mesh.IsValid())
		{
			const MeshHandle bufferHandle = item.mesh;
			const auto vbIt = vertexBuffers->find(bufferHandle);
			const auto ibIt = indexBuffers->find(bufferHandle);
			const auto countIt = indexCounts->find(bufferHandle);

			if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexCounts->end())
			{
				ID3D11Buffer* vb = vbIt->second.Get();
				ID3D11Buffer* ib = ibIt->second.Get();
				const UINT32 fullCount = countIt->second;
				const bool useSubMesh = item.useSubMesh;
				const UINT32 indexCount = useSubMesh ? item.indexCount : fullCount;
				const UINT32 indexStart = useSubMesh ? item.indexStart : 0;

				//DrawMesh(vb, ib, m_RenderContext.inputLayout.Get(), m_RenderContext.VS.Get(), m_RenderContext.PS.Get(), useSubMesh, indexCount, indexStart);
				DrawMesh(vb, ib, vertexShader, pixelShader, useSubMesh, indexCount, indexStart);

				//DrawBones(vertexShader, pixelShader, m_RenderContext.SkinCBuffer.boneCount);
			}
		}
	}

}
