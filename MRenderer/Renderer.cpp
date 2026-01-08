#include "Renderer.h"
#include "OpaquePass.h"
#include "RenderTargetContext.h"

UINT32 GetMaxMeshHandleId(const RenderData::FrameData& frame);

void Renderer::Initialize(HWND hWnd, const RenderData::FrameData& frame, int width, int height)
{
	// Device 생성을 여기서함 (원래는 engine에서 받는 거)
	if (m_bIsInitialized)
		return;

	m_RenderContext.vertexBuffers = &m_vVertexBuffers;
	m_RenderContext.indexBuffers = &m_vIndexBuffers;
	m_RenderContext.indexcounts = &m_vIndexCounts;

	DXSetup(hWnd, width, height); // 멤버함수로 교체

	LoadVertexShader(_T("../MRenderer/fx/Demo_VS.hlsl"), m_pVS.GetAddressOf(), m_pVSCode.GetAddressOf());
	LoadPixelShader(_T("../MRenderer/fx/Demo_PS.hlsl"), m_pPS.GetAddressOf());

	CreateInputLayout(g_pDevice.Get(), m_pVSCode.Get(), m_pInputLayout.GetAddressOf());


	InitVB(frame);// 멤버함수로 교체
	InitIB(frame);// 멤버함수로 교체

	m_Pipeline.AddPass(std::make_unique<OpaquePass>(m_RenderContext, m_AssetLoader));

	UINT size;
	CreateDynamicConstantBuffer(g_pDevice.Get(), sizeof(BaseConstBuffer), m_RenderContext.pBCB.GetAddressOf());
	//이 아래는 확인용
	//ClearBackBuffer(COLOR(0, 0, 1, 1));

	//Flip();
	m_RenderContext.VS = m_pVS;
	m_RenderContext.PS = m_pPS;
	m_RenderContext.VSCode = m_pVSCode;
	m_RenderContext.inputLayout = m_pInputLayout;


	m_bIsInitialized = true;
}

void Renderer::InitializeTest(HWND hWnd, int width, int height)
{
	//Device 생성을 여기서
	if (m_bIsInitialized)
		return;


	m_RenderContext.vertexBuffers = &m_vVertexBuffers;
	m_RenderContext.indexBuffers = &m_vIndexBuffers;
	m_RenderContext.indexcounts = &m_vIndexCounts;

	DXSetup(hWnd, width, height); // 멤버함수로 교체

	LoadVertexShader(_T("../MRenderer/fx/Demo_VS.hlsl"), m_pVS.GetAddressOf(), m_pVSCode.GetAddressOf());
	LoadPixelShader(_T("../MRenderer/fx/Demo_PS.hlsl"), m_pPS.GetAddressOf());

	CreateInputLayout(g_pDevice.Get(), m_pVSCode.Get(), m_pInputLayout.GetAddressOf());


	m_Pipeline.AddPass(std::make_unique<OpaquePass>(m_RenderContext, m_AssetLoader));
	UINT size;
	CreateDynamicConstantBuffer(g_pDevice.Get(), sizeof(BaseConstBuffer), m_RenderContext.pBCB.GetAddressOf());

	m_RenderContext.VS = m_pVS;
	m_RenderContext.PS = m_pPS;
	m_RenderContext.VSCode = m_pVSCode;
	m_RenderContext.inputLayout = m_pInputLayout;


	m_bIsInitialized = true;

}

void Renderer::RenderFrame(const RenderData::FrameData& frame)
{
	m_Pipeline.Execute(frame);
	Flip();
}

void Renderer::RenderFrame(const RenderData::FrameData& frame, RenderTargetContext& rendertargetcontext)
{
	g_pDXDC->OMSetRenderTargets(1, &g_pRTScene, g_pDSViewScene);

	SetViewPort(960, 800);
	float clearColor[4] = { 0.f, 0.f, 1.f, 1.f };
	g_pDXDC->ClearRenderTargetView(g_pRTScene, clearColor);
	g_pDXDC->ClearDepthStencilView(g_pDSViewScene, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);	//새로 생성한 깊이 버퍼

	m_Pipeline.Execute(frame);

	rendertargetcontext.SetShaderResourceView(g_pTexRvScene);
}

void Renderer::InitVB(const RenderData::FrameData& frame)
{
	UINT32 vsize = GetMaxMeshHandleId(frame);
	m_vVertexBuffers.resize(vsize + 1);

	for (const auto& item : frame.renderItems)
	{
		UINT index;
		index = item.mesh.id;
		if (m_vVertexBuffers[index] != nullptr)
			continue;

		RenderData::MeshData* mesh =
			m_AssetLoader.GetMeshes().Get(item.mesh);

		ComPtr<ID3D11Buffer> vb;
		CreateVertexBuffer(g_pDevice.Get(), mesh->vertices.data(), mesh->vertices.size(), sizeof(RenderData::Vertex), vb.GetAddressOf());

		m_vVertexBuffers[index] = vb;
	}
}

//보니까 데이터 형식 상 vb와 함수 하나로 합치는게 좋을듯
void Renderer::InitIB(const RenderData::FrameData& frame)
{
	UINT32 vsize = GetMaxMeshHandleId(frame);
	m_vIndexBuffers.resize(vsize + 1);
	m_vIndexCounts.resize(vsize + 1);

	for (const auto& item : frame.renderItems)
	{
		UINT index;
		index = item.mesh.id;
		if (m_vIndexBuffers[index] != nullptr)
			continue;

		RenderData::MeshData* mesh =
			m_AssetLoader.GetMeshes().Get(item.mesh);


		ComPtr<ID3D11Buffer> ib;
		CreateIndexBuffer(g_pDevice.Get(), mesh->indices.data(), mesh->indices.size(), ib.GetAddressOf());
		m_vIndexBuffers[index] = ib;

		UINT32 cnt = mesh->indices.size();
		m_vIndexCounts[index] = cnt;
	}

}

//핸들 개수 최대값 가져오는 함수
UINT32 GetMaxMeshHandleId(const RenderData::FrameData& frame)
{
	UINT32 maxId = 0;
	for (const auto& item : frame.renderItems)
	{
		if (item.mesh.IsValid() && item.mesh.id > maxId)
		{
			maxId = item.mesh.id;
		}
	}

	return maxId;
}