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
}

void RenderPass::SetBaseCB(const RenderData::RenderItem& item)
{
	m_RenderContext.BCBuffer.mWorld = item.world;
	XMMATRIX world = XMLoadFloat4x4(&item.world);
	XMMATRIX worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(nullptr, world));
	XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorldInvTranspose, worldInvTranspose);

	UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));
}


void RenderPass::SetCameraCB(const RenderData::FrameData& frame)
{
	const auto& context = frame.context;

	if (!m_RenderContext.isEditCam)
	{
		m_RenderContext.CameraCBuffer.mView = context.gameCamera.view;
		m_RenderContext.CameraCBuffer.mProj = context.gameCamera.proj;
		m_RenderContext.CameraCBuffer.mVP = context.gameCamera.viewProj;

		m_RenderContext.pDXDC->OMSetRenderTargets(1, m_RenderContext.pRTView_Imgui.GetAddressOf(), m_RenderContext.pDSViewScene_Depth.Get());

	}
	else if (m_RenderContext.isEditCam)
	{
		m_RenderContext.CameraCBuffer.mView = context.editorCamera.view;
		m_RenderContext.CameraCBuffer.mProj = context.editorCamera.proj;
		m_RenderContext.CameraCBuffer.mVP = context.editorCamera.viewProj;

		m_RenderContext.pDXDC->OMSetRenderTargets(1, m_RenderContext.pRTView_Imgui_edit.GetAddressOf(), m_RenderContext.pDSViewScene_Depth.Get());
	}

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
		dirlight.Intensity = 1.f;
		dirlight.mLightViewProj = light.lightViewProj;
		dirlight.CastShadow = light.castShadow;

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
	ID3D11InputLayout* layout,
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
	dc->IASetInputLayout(layout);
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

bool RenderPass::ShouldIncludeRenderItem(const RenderData::RenderItem& /*item*/) const
{
	return true;
}

void RenderPass::BuildQueue(const RenderData::FrameData& frame)
{
	m_Queue.clear();
	m_Queue.reserve(frame.renderItems.size());

	for (size_t index = 0; index < frame.renderItems.size(); ++index)
	{
		for (const auto& [layer, items] : frame.renderItems)
		{
			if (ShouldIncludeRenderItem(items[index]))
			{
				m_Queue.push_back(index);
			}
		}
	}
}
