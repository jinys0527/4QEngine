#include "ShadowPass.h"

void ShadowPass::Execute(const RenderData::FrameData& frame)
{
    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
    SetRenderTarget(nullptr, m_RenderContext.pDSViewScene_Shadow.Get());
    SetViewPort(m_RenderContext.ShadowTextureSize.width, m_RenderContext.ShadowTextureSize.height, m_RenderContext.pDXDC.Get());
    SetBlendState(BS::DEFAULT);
    SetRasterizerState(RS::CULLBACK);
    SetDepthStencilState(DS::DEPTH_ON);

#pragma endregion
    const auto& context = frame.context;
    if (frame.lights.empty())
        return;

    const auto& mainlight = frame.lights[0];

    //0번이 전역광이라고 가정...
    XMMATRIX lightview, lightproj;
    XMVECTOR maincampos = XMLoadFloat3(&context.gameCamera.cameraPos); //원래는 주인공 위치가 더 좋은데, 일단 카메라 위치로 해도 크게 상관 없을 듯
    XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&mainlight.direction));
    XMVECTOR pos = maincampos - (dir * 10.f);
    XMVECTOR look = maincampos;
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);

    if (XMVector4Equal(pos, look)) return;

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

    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, lightview);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, lightproj);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, lightview* lightproj);

    //★ 나중에 그림자 매핑용 행렬 위치 정해지면 상수 버퍼 set
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mShadow, mLightTM);

    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));


    for (const auto& queueItem : GetQueue())
    {
        const auto& item = *queueItem.item;
        SetBaseCB(item);
        if (m_RenderContext.pSkinCB && item.skinningPaletteCount > 0)
        {
            const size_t paletteStart = item.skinningPaletteOffset;
            const size_t paletteCount = item.skinningPaletteCount;
            const size_t paletteSize = frame.skinningPalettes.size();
            const size_t maxCount = min(static_cast<size_t>(kMaxSkinningBones), paletteCount);
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

#pragma region ShaderSet
        const auto* vertexShaders = m_RenderContext.vertexShaders;

        ID3D11VertexShader* vertexShader = m_RenderContext.VS_PBR.Get();

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
        }
#pragma endregion


        const auto* vertexBuffers = m_RenderContext.vertexBuffers;
        const auto* indexBuffers = m_RenderContext.indexBuffers;
        const auto* indexcounts = m_RenderContext.indexCounts;

        if (vertexBuffers && indexBuffers && indexcounts && item.mesh.IsValid())
        {
            const MeshHandle bufferHandle = item.mesh;
            const auto vbIt = vertexBuffers->find(bufferHandle);
            const auto ibIt = indexBuffers->find(bufferHandle);
            const auto countIt = indexcounts->find(bufferHandle);
            if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexcounts->end())
            {
                ID3D11Buffer* vb = vbIt->second.Get();
                ID3D11Buffer* ib = ibIt->second.Get();
                const bool useSubMesh = item.useSubMesh;
                const UINT32 indexCount = useSubMesh ? item.indexCount : countIt->second;
                const UINT32 indexStart = useSubMesh ? item.indexStart : 0;
                if (vb && ib)
                {
                    DrawMesh(vb, ib, vertexShader, nullptr, useSubMesh, indexCount, indexStart);
                }
            }
        }
    }
}
