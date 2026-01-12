#pragma once

#include "DX11.h"
#include <unordered_map>
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
	void InitializeTest(HWND hWnd, int width, int height, ID3D11Device* device, ID3D11DeviceContext* dxdc);		//Editor의 Renderer 초기화
	void RenderFrame(const RenderData::FrameData& frame);
	void RenderFrame(const RenderData::FrameData& frame, RenderTargetContext& rendertargetcontext);

	void InitVB(const RenderData::FrameData& frame);
	void InitIB(const RenderData::FrameData& frame);

	ComPtr<ID3D11RenderTargetView>	GetRTView() { return m_pRTView; }
	ComPtr<IDXGISwapChain>			GetSwapChain() { return m_pSwapChain; }

//DX Set
public :
	HRESULT ResetRenderTarget(int width, int height);			//화면크기 바꿨을 때 렌더타겟도 그에 맞게 초기화, 테스트 아직 못함
private:
	void	DXSetup(HWND hWnd, int width, int height);
	HRESULT CreateDeviceSwapChain(HWND hWnd);
	HRESULT CreateRenderTarget();
	HRESULT CreateRenderTarget_Other();
	HRESULT ReCreateRenderTarget();
	HRESULT CreateDepthStencil(int width, int height);
	HRESULT	CreateDepthStencilState();
	HRESULT	CreateRasterState();
	HRESULT	CreateSamplerState();
	HRESULT CreateBlendState();
	HRESULT ReleaseScreenSizeResource();						//화면 크기 영향 받는 리소스 해제

	DWORD m_dwAA = 1;				//안티에일리어싱, 1: 안함, 이후 수: 샘플 개수

	ComPtr<ID3D11Device>            m_pDevice;
	ComPtr<ID3D11DeviceContext>     m_pDXDC;
	ComPtr<IDXGISwapChain>			m_pSwapChain;
	ComPtr<ID3D11RenderTargetView>	m_pRTView;
	ComPtr<ID3D11Texture2D>         m_pDS;
	ComPtr<ID3D11DepthStencilView>  m_pDSView;


	//imgui용
	ComPtr<ID3D11Texture2D>				m_pRTScene_Imgui;
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_Imgui;
	ComPtr<ID3D11RenderTargetView>		m_pRTView_Imgui;

	ComPtr<ID3D11Texture2D>				m_pDSTex_Imgui;
	ComPtr<ID3D11DepthStencilView>		m_pDSViewScene_Imgui;

	//그림자 매핑용
	ComPtr<ID3D11Texture2D>				m_pDSTex_Shadow;
	ComPtr<ID3D11DepthStencilView>		m_pDSViewScene_Shadow;
	ComPtr<ID3D11ShaderResourceView>	m_pShadowRV;
	TextureSize							m_ShadowTextureSize = { 0,0 };

	//DepthPass용
	ComPtr<ID3D11Texture2D>				m_pDSTex_Depth;
	ComPtr<ID3D11DepthStencilView>		m_pDSViewScene_Depth;
	ComPtr<ID3D11ShaderResourceView>	m_pDepthRV;

	//PostPass용
	ComPtr<ID3D11Texture2D>				m_pRTScene_Post;
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_Post;
	ComPtr<ID3D11RenderTargetView>		m_pRTView_Post;



	EnumArray<ComPtr<ID3D11DepthStencilState>, static_cast<size_t>(DS::MAX_)>	m_DSState;		//깊이 스텐실 상태
	EnumArray<ComPtr<ID3D11RasterizerState>, static_cast<size_t>(RS::MAX_)>		m_RState;		//래스터라이저 상태
	EnumArray<ComPtr<ID3D11SamplerState>, static_cast<size_t>(SS::MAX_)>		m_SState;		//샘플러 상태
	EnumArray<ComPtr<ID3D11BlendState>, static_cast<size_t>(BS::MAX_)>			m_BState;		//블렌딩 상태

private:
	void CreateContext();

	//DXHelper
	HRESULT Compile(const WCHAR* FileName, const char* EntryPoint, const char* ShaderModel, ID3DBlob** ppCode);
	HRESULT LoadVertexShader(const TCHAR* filename, ID3D11VertexShader** ppVS, ID3DBlob** ppVSCode);
	HRESULT LoadPixelShader(const TCHAR* filename, ID3D11PixelShader** ppPS);
	HRESULT CreateInputLayout();			//일단 하나만, 나중에 레이아웃 추가되면 함수를 추가하든 여기서 추가하든 하면될듯
	HRESULT CreateConstBuffer();
	HRESULT CreateVertexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, UINT stride, ID3D11Buffer** ppVB);
	HRESULT CreateIndexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, ID3D11Buffer** ppIB);
	HRESULT CreateConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB);
	HRESULT RTTexCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex);
	HRESULT RTViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView);
	HRESULT RTSRViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV);
	HRESULT DSCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView);
	HRESULT RTCubeTexCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex);
	HRESULT CubeRTViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView, UINT faceIndex);
	HRESULT RTCubeSRViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV);
private:
	bool m_bIsInitialized = false;

	TextureSize m_WindowSize = { 0,0 };

	RenderPipeline m_Pipeline;
	AssetLoader& m_AssetLoader;


	//렌더링에 필요한 요소들 공유를 위한 소통 창구 
	RenderContext m_RenderContext;

	//버텍스버퍼들
	//※ map으로 관리 or 다른 방식 사용. notion issue 참조※
	std::unordered_map<UINT, ComPtr<ID3D11Buffer>>	m_VertexBuffers;
	std::unordered_map<UINT, ComPtr<ID3D11Buffer>>	m_IndexBuffers;
	std::unordered_map<UINT, UINT32>				m_IndexCounts;

	//임시
	ComPtr<ID3D11InputLayout> m_pInputLayout;			
	//임시 쉐이더코드
	ComPtr<ID3D11VertexShader> m_pVS;
	ComPtr<ID3D11PixelShader> m_pPS;
	ComPtr<ID3DBlob> m_pVSCode;


	//Shadow, Depth용
	ComPtr<ID3D11InputLayout> m_pInputLayout_P;
	ComPtr<ID3D11VertexShader> m_pVS_P;
	ComPtr<ID3DBlob> m_pVSCode_P;


//그리드
private:
	struct VertexPC { XMFLOAT3 pos;};

	ComPtr<ID3D11Buffer> m_GridVB;
	//ComPtr<ID3D11InputLayout> m_pInputLayoutGrid;

	UINT m_GridVertexCount = 0;
	std::vector<std::vector<int>> m_GridFlags;  // 0=empty,1=blocked
	void CreateGridVB();
	void UpdateGrid(const RenderData::FrameData& frame);
	void DrawGrid();
	float m_CellSize = 1.0f;
	int   m_HalfCells = 20;

};
