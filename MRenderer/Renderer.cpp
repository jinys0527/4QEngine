#include "Renderer.h"
#include "OpaquePass.h"

UINT32 GetMaxMeshHandleId(const RenderData::FrameData& frame);

void Renderer::Initialize(HWND hWnd, const RenderData::FrameData& frame, int width, int height)
{
	m_RenderContext.vertexBuffers = &m_vVertexBuffers;
	m_RenderContext.indexBuffers = &m_vIndexBuffers;
	m_RenderContext.indexcounts = &m_vIndexCounts;

	DXSetup(hWnd, width, height);

	InitVB(frame);
	InitIB(frame);

	m_Pipeline.AddPass(std::make_unique<OpaquePass>(m_RenderContext, m_AssetLoader));

	UINT size;
	CreateDynamicConstantBuffer(g_pDevice.Get(), sizeof(BaseConstBuffer), m_RenderContext.pBCB.GetAddressOf());


	

	//이 아래는 확인용
	ClearBackBuffer(COLOR(0, 0, 1, 1));

	Flip();
}

void Renderer::RenderFrame(const RenderData::FrameData& frame)
{
	m_Pipeline.Execute(frame);
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