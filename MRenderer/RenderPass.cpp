#include "RenderPass.h"

void RenderPass::Setup(const RenderData::FrameData& frame)
{
	BuildQueue(frame);
}



void RenderPass::SetBlendState(BS state)
{
	//if (state == BS::MAX_) // 예시: MAX_를 '기본 상태' 용도로 쓰고 싶다면
	//{
	//	m_RenderContext.pDXDC->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	//	return;
	//}
	m_RenderContext.pDXDC->OMSetBlendState(m_RenderContext.BState[state].Get(), nullptr, 0xFFFFFFFF);
}

void RenderPass::SetDepthStencilState(DS state)
{
	m_RenderContext.pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[state].Get(), 0);
}

void RenderPass::SetRasterizerState(RS state)
{
	m_RenderContext.pDXDC->RSSetState(m_RenderContext.RState[state].Get());
}

void RenderPass::SetSamplerState()
{
	m_RenderContext.pDXDC->PSSetSamplers(0, 1, m_RenderContext.SState[SS::WRAP].GetAddressOf());
	m_RenderContext.pDXDC->PSSetSamplers(1, 1, m_RenderContext.SState[SS::MIRROR].GetAddressOf());
	m_RenderContext.pDXDC->PSSetSamplers(2, 1, m_RenderContext.SState[SS::CLAMP].GetAddressOf());
	m_RenderContext.pDXDC->PSSetSamplers(3, 1, m_RenderContext.SState[SS::BORDER].GetAddressOf());
	m_RenderContext.pDXDC->PSSetSamplers(4, 1, m_RenderContext.SState[SS::BORDER_SHADOW].GetAddressOf());
}

//클리어까지 동시 실행
void RenderPass::SetRenderTarget(ID3D11RenderTargetView* rtview, ID3D11DepthStencilView* dsview)
{
	float clearColor[4] = { 0.21f, 0.21f, 0.21f, 1.f };

	if (rtview)
	{
		m_RenderContext.pDXDC->OMSetRenderTargets(1, &rtview, dsview);
		m_RenderContext.pDXDC->ClearRenderTargetView(rtview, clearColor);
	}
	else
	{
		m_RenderContext.pDXDC->OMSetRenderTargets(0, nullptr, dsview);
	}

	if (dsview)
	{
		m_RenderContext.pDXDC->ClearDepthStencilView(
			dsview,
			D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
			1.0f,
			0
		);
	}
}

void RenderPass::SetBaseCB(const RenderData::RenderItem& item)
{
	XMMATRIX mtm = XMLoadFloat4x4(&item.world);
	XMMATRIX mLocalToWorld = XMLoadFloat4x4(&item.localToWorld);

	mtm = MathUtils::Mul(mLocalToWorld, mtm);

	XMFLOAT4X4 tm;
	XMStoreFloat4x4(&tm, mtm);

	m_RenderContext.BCBuffer.mWorld = tm;

	XMMATRIX world = XMLoadFloat4x4(&tm);
	XMMATRIX worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, world));
	XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorldInvTranspose, worldInvTranspose);

	UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));
}

//벽뚫 마스킹맵용 행렬
void RenderPass::SetMaskingTM(const RenderData::RenderItem& item, const XMFLOAT3& campos)
{
	if (!m_RenderContext.isEditCam)
	{
		XMMATRIX mTM, mView, mProj;
		XMVECTOR maincampos = XMLoadFloat3(&campos); 
		XMMATRIX targetworld = XMLoadFloat4x4(&item.world);
		XMVECTOR targetpos = targetworld.r[3];
		XMVECTOR look = targetpos;
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);

		//if (XMVector4Equal(maincampos, look)) return;
		mView = XMMatrixLookAtLH(maincampos, look, up);
		mProj = XMMatrixOrthographicLH(8, 8, 0.1f, 200.f);

		XMFLOAT4X4 m = {
		0.5f,  0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f,  0.0f, 1.0f, 0.0f,
		0.5f,  0.5f, 0.0f, 1.0f
		};

		XMMATRIX mscale = XMLoadFloat4x4(&m);
		mTM = mView * mProj * mscale;

		XMStoreFloat4x4(&m_RenderContext.BCBuffer.mTextureMask, mTM);

		m_RenderContext.BCBuffer.mTextureMask;

		UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));
	}
}


void RenderPass::SetCameraCB(const RenderData::FrameData& frame)
{
	const auto& context = frame.context;

	if (!m_RenderContext.isEditCam)
	{
		m_RenderContext.CameraCBuffer.mView = context.gameCamera.view;
		m_RenderContext.CameraCBuffer.mProj = context.gameCamera.proj;
		m_RenderContext.CameraCBuffer.mVP = context.gameCamera.viewProj;
		m_RenderContext.CameraCBuffer.camPos = context.gameCamera.cameraPos;
	}
	else if (m_RenderContext.isEditCam)
	{
		m_RenderContext.CameraCBuffer.mView = context.editorCamera.view;
		m_RenderContext.CameraCBuffer.mProj = context.editorCamera.proj;
		m_RenderContext.CameraCBuffer.mVP = context.editorCamera.viewProj;
		m_RenderContext.CameraCBuffer.camPos = context.editorCamera.cameraPos;
	}

	//스카이박스 행렬
#pragma region SkyBox
	XMFLOAT4X4 view, proj;
	view = m_RenderContext.CameraCBuffer.mView;
	proj = m_RenderContext.CameraCBuffer.mProj;
	view._41 = view._42 = view._43 = 0.0f;

	XMMATRIX mV, mP, mVP, mSkyBox;
	mV = XMLoadFloat4x4(&view);
	mP = XMLoadFloat4x4(&proj);

	mVP = mV * mP;
	mSkyBox = XMMatrixInverse(nullptr, mVP);
	XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mSkyBox, mSkyBox);
	//스카이박스 행렬 끝

#pragma endregion

	UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));
}

void RenderPass::SetDirLight(const RenderData::FrameData& frame)
{
	if (!frame.lights.empty())
	{
		const auto& light = frame.lights[0];

		XMFLOAT4X4 view; 
		if (m_RenderContext.isEditCam)
		{
			view = frame.context.editorCamera.view;
		}
		else if (!m_RenderContext.isEditCam)
		{
			view = frame.context.gameCamera.view;
		}
		XMMATRIX mView = XMLoadFloat4x4(&view);

		Light dirlight{};
		dirlight.worldDir = XMFLOAT3(-light.direction.x, -light.direction.y, -light.direction.z);

		XMVECTOR dirW = XMLoadFloat3(&dirlight.worldDir); 
		XMVECTOR dirV = XMVector3Normalize(XMVector3TransformNormal(dirW, mView));
		XMStoreFloat3(&dirlight.viewDir, dirV);

		dirlight.Color = XMFLOAT4(light.color.x, light.color.y, light.color.z, 1);
		dirlight.Intensity = 3.14f;
		dirlight.mLightViewProj = light.lightViewProj;
		dirlight.CastShadow = light.castShadow;
		dirlight.Range = light.range;
		dirlight.SpotInnerAngle = light.spotInnerAngle;
		dirlight.SpotOutterAngle = light.spotOutterAngle;
		dirlight.AttenuationRadius = light.attenuationRadius;

		m_RenderContext.LightCBuffer.lights[0] = dirlight;

		UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pLightCB.Get(), &m_RenderContext.LightCBuffer, sizeof(LightConstBuffer));

	}

}

void RenderPass::SetVertex(const RenderData::RenderItem& item)
{
	const auto* vertexBuffers = m_RenderContext.vertexBuffers;
	const auto* indexBuffers = m_RenderContext.indexBuffers;
	const auto* indexCounts = m_RenderContext.indexCounts;
	const auto* textures = m_RenderContext.textures;

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
		}
	}
}

void RenderPass::DrawMesh(
	ID3D11Buffer* vb,
	ID3D11Buffer* ib,
	ID3D11VertexShader* vs,
	ID3D11PixelShader* ps,
	BOOL useSubMesh,
	UINT indexCount,
	UINT indexStart
)
{
	const UINT stride = sizeof(RenderData::Vertex);
	const UINT offset = 0;

	ID3D11DeviceContext* dc = m_RenderContext.pDXDC.Get();

	dc->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	dc->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
	dc->IASetInputLayout(m_RenderContext.InputLayout.Get());
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	dc->VSSetShader(vs, nullptr, 0);
	dc->PSSetShader(ps, nullptr, 0);

	dc->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->PSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->VSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->PSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->VSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());
	dc->PSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());


	if (useSubMesh)
	{
		dc->DrawIndexed(indexCount, indexStart, 0);
	}
	else
	{
		dc->DrawIndexed(indexCount, 0, 0);
	}
}

void RenderPass::DrawBones(ID3D11VertexShader* vs, ID3D11PixelShader* ps, UINT boneCount)
{
	ID3D11DeviceContext* dc = m_RenderContext.pDXDC.Get();

	// VB/IB 안 씀
	dc->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
	dc->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

	// InputLayout 필요 없음 (SV_VertexID만 사용)
	dc->IASetInputLayout(nullptr);

	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	dc->VSSetShader(vs, nullptr, 0);
	dc->PSSetShader(ps, nullptr, 0);

	// 기존 CB들
	dc->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->PSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->VSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->PSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->VSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());
	dc->PSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());

	// 본 N개 -> 라인 버텍스 2N개
	dc->Draw(boneCount * 2, 0);
}

bool RenderPass::ShouldIncludeRenderItem(RenderData::RenderLayer /*layer*/, const RenderData::RenderItem& /*item*/) const 
{
	return false;
}

void RenderPass::BuildQueue(const RenderData::FrameData& frame)
{
	m_Queue.clear();
	size_t totalItems = 0;
	for (const auto& [layer, items] : frame.renderItems)
	{
		totalItems += items.size();
	}
	m_Queue.reserve(totalItems);


	for (const auto& [layer, items] : frame.renderItems) 
	{
		for (const auto& item : items) 
		{
			if (ShouldIncludeRenderItem(layer, item))
			{
				m_Queue.push_back({ layer, &item });
			}
		}
	}
}
