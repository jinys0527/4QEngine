#pragma once

#include "DX11.h"
#include "Buffers.h"
#include "RenderPipeline.h"

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

	void InitVB(const RenderData::FrameData& frame);
	void InitIB(const RenderData::FrameData& frame);

private:
	RenderPipeline m_Pipeline;
	AssetLoader& m_AssetLoader;

	//상수버퍼들
	RenderContext m_RenderContext;

	//버텍스버퍼들
	//※ map으로 관리 or 다른 방식 사용. notion issue 참조※
	std::vector<ComPtr<ID3D11Buffer>> m_vVertexBuffers;
	std::vector<ComPtr<ID3D11Buffer>> m_vIndexBuffers ;
	std::vector<UINT32>				  m_vIndexCounts  ;
	// RT
	// 
};
