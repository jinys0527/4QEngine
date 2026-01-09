#pragma once

#include "DX11.h"
#include "Buffers.h"
#include "RenderPipeline.h"
#include "RenderTargetContext.h"

// 렌더링 진입점 클래스
// RenderPipeline을 관리한다.
// 
class Renderer
{
public:
	Renderer(AssetLoader& assetloader) : m_AssetLoader(assetloader) {}
	RenderPipeline& GetPipeline() { return m_Pipeline; }
	const RenderPipeline& GetPipeline() const { return m_Pipeline; }

	void Initialize(HWND hWnd, const RenderData::FrameData& frame, int width, int height);
	void InitializeTest(HWND hWnd, int width, int height);
	void RenderFrame(const RenderData::FrameData& frame);
	void RenderFrame(const RenderData::FrameData& frame, RenderTargetContext& rendertargetcontext);

	void InitVB(const RenderData::FrameData& frame);
	void InitIB(const RenderData::FrameData& frame);

private:
	bool m_bIsInitialized = false;

	RenderPipeline m_Pipeline;
	AssetLoader& m_AssetLoader;

	//상수버퍼들
	RenderContext m_RenderContext;

	//버텍스버퍼들
	//※ map으로 관리 or 다른 방식 사용. notion issue 참조※
	std::vector<ComPtr<ID3D11Buffer>> m_vVertexBuffers;
	std::vector<ComPtr<ID3D11Buffer>> m_vIndexBuffers ;
	std::vector<UINT32>				  m_vIndexCounts  ;
	
	//임시
	ComPtr<ID3D11InputLayout> m_pInputLayout;
	//임시 쉐이더코드
	ComPtr<ID3D11VertexShader> m_pVS;
	ComPtr<ID3D11PixelShader> m_pPS;
	ComPtr<ID3DBlob> m_pVSCode;

//그리드
private:
	struct VertexPC { XMFLOAT3 pos;};

	ComPtr<ID3D11Buffer> m_GridVB;
	UINT m_GridVertexCount = 0;
	std::vector<std::vector<int>> m_GridFlags;  // 0=empty,1=blocked
	void CreateGridVB();
	void UpdateGrid(const RenderData::FrameData& frame);
	void DrawGrid();
	float m_CellSize = 1.0f;
	int   m_HalfCells = 20;

};
