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

	CreateDynamicConstantBuffer(g_pDevice.Get(), sizeof(BaseConstBuffer), m_RenderContext.pBCB.GetAddressOf());
	//이 아래는 확인용
	//ClearBackBuffer(COLOR(0, 0, 1, 1));

	//Flip();
	m_RenderContext.VS = m_pVS;
	m_RenderContext.PS = m_pPS;
	m_RenderContext.VSCode = m_pVSCode;
	m_RenderContext.inputLayout = m_pInputLayout;

	//그리드
	CreateGridVB();
	const int gridSize = m_HalfCells * 2 + 1;
	m_GridFlags.assign(gridSize, std::vector<int>(gridSize, 0));



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
	CreateDynamicConstantBuffer(g_pDevice.Get(), sizeof(BaseConstBuffer), m_RenderContext.pBCB.GetAddressOf());

	m_RenderContext.VS = m_pVS;
	m_RenderContext.PS = m_pPS;
	m_RenderContext.VSCode = m_pVSCode;
	m_RenderContext.inputLayout = m_pInputLayout;

	//그리드
	CreateGridVB();
	const int gridSize = m_HalfCells * 2 + 1;
	m_GridFlags.assign(gridSize, std::vector<int>(gridSize, 0));


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

	//그리드
	UpdateGrid(frame);
	DrawGrid();
	
	m_Pipeline.Execute(frame);

	rendertargetcontext.SetShaderResourceView(g_pTexRvScene);
}

void Renderer::InitVB(const RenderData::FrameData& frame)
{
	UINT32 vsize = GetMaxMeshHandleId(frame);
	m_vVertexBuffers.resize(vsize + 1);

	for (const auto& [layer, items] : frame.renderItems)
	{
		for (const auto& item : items)
		{
			UINT index;
			index = item.mesh.id;
			if (m_vVertexBuffers[index] != nullptr)
				continue;

			RenderData::MeshData* mesh =
				m_AssetLoader.GetMeshes().Get(item.mesh);

			ComPtr<ID3D11Buffer> vb;
			CreateVertexBuffer(g_pDevice.Get(), mesh->vertices.data(), static_cast<UINT>(mesh->vertices.size()), sizeof(RenderData::Vertex), vb.GetAddressOf());
			 
			m_vVertexBuffers[index] = vb;
		}
	}
}

//보니까 데이터 형식 상 vb와 함수 하나로 합치는게 좋을듯
void Renderer::InitIB(const RenderData::FrameData& frame)
{
	UINT32 vsize = GetMaxMeshHandleId(frame);
	m_vIndexBuffers.resize(vsize + 1);
	m_vIndexCounts.resize(vsize + 1);

	for (const auto& [layer, items] : frame.renderItems)
	{
		for (const auto& item : items)
		{
			UINT index;
			index = item.mesh.id;
			if (m_vIndexBuffers[index] != nullptr)
				continue;

			RenderData::MeshData* mesh =
				m_AssetLoader.GetMeshes().Get(item.mesh);


			ComPtr<ID3D11Buffer> ib;
			CreateIndexBuffer(g_pDevice.Get(), mesh->indices.data(), static_cast<UINT>(mesh->indices.size()), ib.GetAddressOf());
			m_vIndexBuffers[index] = ib;

			UINT32 cnt = static_cast<UINT32>(mesh->indices.size());
			m_vIndexCounts[index] = cnt;
		}
	}
}

void Renderer::CreateGridVB()
{
	const int   N = m_HalfCells;
	const float s = m_CellSize;
	const float half = N * s;

	std::vector<VertexPC> v;
	v.reserve((N * 2 + 1) * 4);

	XMFLOAT3 cMajor(1, 1, 1), cMinor(0.7f, 0.7f, 0.7f);
	XMFLOAT3 cAxisX(0.8f, 0.2f, 0.2f), cAxisZ(0.2f, 0.4f, 0.8f);

	for (int i = -N; i <= N; ++i) 
	{ 
		float x = i * s;
		//XMFLOAT3 col = (i == 0) ? cAxisZ : ((i % 5 == 0) ? cMajor : cMinor);
		v.push_back({ XMFLOAT3(-half, 0, x)});
		v.push_back({ XMFLOAT3(+half, 0, x)});

		float z = i * s;
		//col = (i == 0) ? cAxisX : ((i % 5 == 0) ? cMajor : cMinor);
		v.push_back({ XMFLOAT3(z, 0, -half)});
		v.push_back({ XMFLOAT3(z, 0, +half)});
	}

	m_GridVertexCount = (UINT)v.size();

	D3D11_BUFFER_DESC bd{};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = UINT(v.size() * sizeof(VertexPC));
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	D3D11_SUBRESOURCE_DATA sd{ v.data(), 0, 0 };
	g_pDevice->CreateBuffer(&bd, &sd, m_GridVB.GetAddressOf());
}

void Renderer::UpdateGrid(const RenderData::FrameData& frame)
{
	const auto& context = frame.context;

	//m_RenderContext.BCBuffer.mView = context.view;
	//m_RenderContext.BCBuffer.mProj = context.proj;
	//m_RenderContext.BCBuffer.mVP = context.viewProj;

	XMFLOAT4X4 tm;

	XMFLOAT3		g_vEye(0.0f, 10.0f, -6.0f);		//카메라 위치1.(Position)
	XMFLOAT3		g_vLookAt(0.0f, 0.0f, 0.0f);	//바라보는 곳1.(Position)
	XMFLOAT3		g_vUp(0.0f, 1.0f, 0.0f);		//카메라 상방 벡터1.(Direction)

	// 투영 변환 정보. 
	float g_fFov = XMConvertToRadians(45);	//기본 FOV 앵글. Field of View (Y) 

	XMVECTOR eye = XMLoadFloat3(&g_vEye);	//카메라 위치 
	XMVECTOR lookat = XMLoadFloat3(&g_vLookAt);	//바라보는 곳.위치.
	XMVECTOR up = XMLoadFloat3(&g_vUp);		//카메라 상방 벡터.	
	// 뷰 변환 행렬 생성 :  View Transform 
	XMMATRIX mView = XMMatrixLookAtLH(eye, lookat, up);

	XMMATRIX mProj = XMMatrixPerspectiveFovLH(g_fFov, 960.f / 800.f, 1, 300);

	XMStoreFloat4x4(&tm, mView);
	m_RenderContext.BCBuffer.mView = tm;


	XMStoreFloat4x4(&tm, mProj);
	m_RenderContext.BCBuffer.mProj = tm;

	XMStoreFloat4x4(&tm, mView * mProj);
	m_RenderContext.BCBuffer.mVP = tm;


	XMFLOAT4X4 wTM;
	XMStoreFloat4x4(&wTM, XMMatrixIdentity());
	m_RenderContext.BCBuffer.mWorld = wTM;

	UpdateDynamicBuffer(g_pDXDC.Get(), m_RenderContext.pBCB.Get(), &m_RenderContext.BCBuffer, sizeof(BaseConstBuffer));
}

void Renderer::DrawGrid()
{
	g_pDXDC->IASetInputLayout(m_pInputLayout.Get());
	g_pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	UINT strideC = sizeof(VertexPC), offsetC = 0;
	g_pDXDC->IASetVertexBuffers(0, 1, m_GridVB.GetAddressOf(), &strideC, &offsetC);
	g_pDXDC->VSSetShader(m_pVS.Get(), nullptr, 0);
	g_pDXDC->PSSetShader(m_pPS.Get(), nullptr, 0);
	g_pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());

	g_pDXDC->Draw(m_GridVertexCount, 0);

}

//핸들 개수 최대값 가져오는 함수
UINT32 GetMaxMeshHandleId(const RenderData::FrameData& frame)
{
	UINT32 maxId = 0;
	for (const auto& [layer, items] : frame.renderItems)
	{
		for (const auto& item : items)
		{
			if (item.mesh.IsValid() && item.mesh.id > maxId)
			{
				maxId = item.mesh.id;
			}
		}
	}

	return maxId;
}