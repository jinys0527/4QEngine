#include <iostream>
#include <d3dcompiler.h>
#include "DX11.h"

ComPtr<ID3D11Device>            g_pDevice;
ComPtr<ID3D11DeviceContext>     g_pDXDC;
ComPtr<IDXGISwapChain>          g_pSwapChain; 
ComPtr<ID3D11RenderTargetView>  g_pRTView; 
ComPtr<ID3D11Texture2D>         g_pDS;


DXGI_ADAPTER_DESC1 g_Adc;

BOOL g_bVSync = FALSE;

//Depth/Stencil
ComPtr<ID3D11DepthStencilView> g_pDSView;
EnumArray<ComPtr<ID3D11DepthStencilState>, static_cast<size_t>(DS::MAX_)> g_DSState; 
EnumArray<ComPtr<ID3D11RasterizerState>, static_cast<size_t>(RS::MAX_)> g_RState;

int  DepthStencilStateCreate();


HRESULT CreateDeviceSwapChain(HWND hWnd);
HRESULT CreateRenderTarget();
HRESULT CreateDepthStencil(int width, int height);

//모니터 해상도 관련
ComPtr<IDXGIAdapter> g_pAdapter;
//ComPtr<IDXGIOutput> g_pOutput;
int g_MonitorWidth = 0;
int g_MonitorHeight = 0;


//안티에일리어싱
DWORD		g_dwAA = 1;
DWORD		g_dwAF = 1;
BOOL		g_bMipMap = TRUE;

ID3D11Texture2D*            g_pTexScene = nullptr;			//장면이 렌더링될 새 렌더타겟용 텍스쳐 (리소스)
ID3D11ShaderResourceView*   g_pTexRvScene = nullptr;		//장면이 렌더링될 새 렌더타겟용 텍스쳐-리소스뷰 (멥핑용)
ID3D11RenderTargetView*     g_pRTScene = nullptr;			//장면이 렌더링될 새 렌더타겟뷰 (렌더링용) 

ID3D11Texture2D*        g_pDSTexScene  = nullptr;	//렌더타겟용 깊이/스텐실 버퍼 (텍스처)
ID3D11DepthStencilView* g_pDSViewScene = nullptr;	//렌더타겟용 깊이/스텐실 뷰


//bool DXSetup(HWND hWnd)
//{
//    CreateDeviceSwapChain(hWnd);
//    CreateRenderTarget();
//    CreateDepthStencil();
//    //스텐실 넣으면 바뀌어야함
//    //nullptr부분에 뎁스스텐실뷰 포인터 넣기
//    g_pDXDC->OMSetRenderTargets(1, g_pRTView.GetAddressOf(), g_pDSView.Get());
//    SetViewPort();
//    DepthStencilStateCreate();
//    return true;
//}

bool DXSetup(HWND hWnd, int width, int height)
{
    CreateDeviceSwapChain(hWnd);
    CreateRenderTarget();
    CreateDepthStencil(width, height);
    //스텐실 넣으면 바뀌어야함
    //nullptr부분에 뎁스스텐실뷰 포인터 넣기
    g_pDXDC->OMSetRenderTargets(1, g_pRTView.GetAddressOf(), g_pDSView.Get());
    SetViewPort(width, height);
    DepthStencilStateCreate();


    DXGI_FORMAT fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
    //1. 렌더 타겟용 빈 텍스처로 만들기.	
    RTTexCreate(960, 800, fmt, &g_pTexScene);

    //2. 렌더타겟뷰 생성.
    RTViewCreate(fmt, g_pTexScene, &g_pRTScene);

    //3. 렌더타겟 셰이더 리소스뷰 생성 (멥핑용)
    RTSRViewCreate(fmt, g_pTexScene, &g_pTexRvScene);

    DXGI_FORMAT dsFmt = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;		//원본 DS 포멧 유지.
    DSCreate(960, 800, dsFmt, g_pDSTexScene, g_pDSViewScene);

    return true;
}


void Draw()
{
    float clearColor[4] = { 0.2f, 0.2f, 0.6f, 1.0f }; // 파란색 배경
    g_pDXDC->ClearRenderTargetView(g_pRTView.Get(), clearColor);

    g_pSwapChain->Present(1, 0);
}

void DXRelease()
{
    if (g_pDXDC)
    {
        g_pDXDC->ClearState();
    }
    g_pRTView->Release();
    g_pSwapChain->Release();
    g_pDXDC->Release();
    g_pDevice->Release();

}

int ClearBackBuffer(COLOR col)
{
    g_pDXDC->ClearRenderTargetView(g_pRTView.Get(), (float*)&col);

    return S_OK;
}


int ClearBackBuffer(UINT flag, COLOR col, float depth, UINT stencil)
{
    g_pDXDC->ClearRenderTargetView(g_pRTView.Get(), (float*)&col);
    g_pDXDC->ClearDepthStencilView(g_pDSView.Get(), flag, depth, stencil);	//깊이/스텐실 지우기.

    return 0;
}

int Flip()
{
    g_pSwapChain->Present(g_bVSync, 0);			//화면출력 : Flip! (+수직동기화)

    return 0;
}

void GetDeviceInfo()
{
    //GetFeatureLevel();

    GetAdapterInfo(&g_Adc);

    IDXGIDevice* dxgiDevice = nullptr;
    g_pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

    dxgiDevice->GetAdapter(g_pAdapter.GetAddressOf());

    IDXGIOutput* output = nullptr;
    g_pAdapter->EnumOutputs(0, &output);

    DXGI_OUTPUT_DESC desc;
    output->GetDesc(&desc);

    g_MonitorWidth = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
    g_MonitorHeight = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;


}

HRESULT GetAdapterInfo(DXGI_ADAPTER_DESC1* pAd)
{
    IDXGIAdapter1* pAdapter;
    IDXGIFactory1* pFactory = NULL;

    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&pFactory))))
    {
        return E_FAIL;
    }

    pFactory->EnumAdapters1(0, &pAdapter);		//어뎁터 획득.
    pAdapter->GetDesc1(pAd);					//어뎁터 정보 획득.
    //*pAd = ad;								//외부로 복사.

    //정보 취득후, 접근한 인터페이스 해제. (메모리 누수 방지)
    SafeRelease(pAdapter);
    SafeRelease(pFactory);

    return S_OK;
}

void SystemUpdate(float dTime)
{
    //bool before = *fullscreen;
    //g_pSwapChain->GetFullscreenState(fullscreen, nullptr);

    //if (IsKeyUp(VK_SPACE))	g_bWireFrame ^= TRUE;
    //if (IsKeyUp(VK_F4))		g_bCullBack ^= TRUE;


    //if (g_bWireFrame) g_BkColor = g_ColDGray;
    //else			  g_BkColor = g_ColGray;


    //// 렌더링 모드 전환.	  
    //RenderModeUpdate();

    //// 깊이 연산 모드 전환.	 
    //if (g_bZEnable)
    //    g_pDXDC->OMSetDepthStencilState(g_DSState[DS_DEPTH_ON], 0);	//깊이 버퍼 동작 (기본값) 
    //else  g_pDXDC->OMSetDepthStencilState(g_DSState[DS_DEPTH_OFF], 0);	//깊이 버퍼 비활성화 : Z-Test Off + Z-Write Off.

    g_pDXDC->OMSetDepthStencilState(g_DSState[DS::OFF].Get(), 0);

}

int CreateBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppBuff)
{
    return S_OK;
}

int UpdateBuffer(ID3D11Buffer* pBuff, LPVOID pData, UINT size)
{
    return S_OK;
}

int CreateVertexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, UINT stride, ID3D11Buffer** ppVB)
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;			
    bd.ByteWidth = size;							
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;		
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA rd;
    ZeroMemory(&rd, sizeof(rd));
    rd.pSysMem = pData;									

    ID3D11Buffer* pVB = nullptr;
    hr = g_pDevice->CreateBuffer(&bd, &rd, &pVB);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    *ppVB = pVB;

    return S_OK;
}

int CreateIndexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, ID3D11Buffer** ppIB)
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC bd = {};

    bd.Usage = D3D11_USAGE_DEFAULT;			
    bd.ByteWidth = size;					
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;	
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA rd;
    ZeroMemory(&rd, sizeof(rd));
    rd.pSysMem = pData;									

    ID3D11Buffer* pIB = nullptr;
    hr = g_pDevice->CreateBuffer(&bd, &rd, &pIB);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    *ppIB = pIB;

    return S_OK;
}

int CreateConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB)
{
    HRESULT hr = S_OK;

    DWORD sizeAligned = AlignCBSize(size);

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;				
    bd.ByteWidth = sizeAligned;						
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;		

    ID3D11Buffer* pCB = nullptr;
    hr = pDev->CreateBuffer(&bd, nullptr, &pCB);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    *ppCB = pCB;

    return S_OK;
}

DWORD AlignCBSize(DWORD size)
{
    DWORD sizeAligned = 0;
    BOOL bAligned = (size % 16) ? FALSE : TRUE;		//정렬(필요) 확인.
    TCHAR dbgMsg[256] = _T("");						//디버깅 메세지.


    if (bAligned)
    {
        sizeAligned = size;

        //_stprintf(dbgMsg, _T("[알림] 상수버퍼 : 16바이트 정렬됨. \n> ConstBuffer = %d \n> 필요 정렬 크기 = %d"), size, sizeAligned);
    }
    else
    {
        sizeAligned = (size / 16) * 16 + 16;		//정렬(필요) 크기 재산출.

        //_stprintf(dbgMsg, _T("[경고] 상수버퍼 : 16바이트 미정렬. \n> ConstBuffer = %d \n> 필요 정렬 크기 = %d"), size, sizeAligned);
    }

    return sizeAligned;
}

int CreateInputLayout(ID3D11Device* pDev, D3D11_INPUT_ELEMENT_DESC* ed, DWORD num, ID3DBlob* pVSCode, ID3D11InputLayout** ppLayout)
{
    HRESULT hr = S_OK;

    // 정접 입력구조 객체 생성 Create the input layout
    // 함께 사용될 셰이더(컴파일된 바이너리 코드)가 필요합니다.
    ID3D11InputLayout* pLayout = nullptr;
    hr = pDev->CreateInputLayout(ed, num, pVSCode->GetBufferPointer(), pVSCode->GetBufferSize(), &pLayout);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    //외부로 리턴.
    *ppLayout = pLayout;

    return S_OK;
}

//초기값 없이 버퍼만 만들 때(크기 고정)
HRESULT CreateDynamicConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB)
{
    HRESULT hr = S_OK;
    ID3D11Buffer* pCB = nullptr;

    //정렬된 버퍼 크기 계산 
    DWORD sizeAligned = AlignCBSize(size);

    //상수 버퍼 정보 설정.
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;				//동적 정점버퍼 설정.
    bd.ByteWidth = sizeAligned;						//버퍼 크기 : 128비트 정렬 추가.
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;				//CPU 접근 설정. 

    /*//서브리소스 설정.
    D3D11_SUBRESOURCE_DATA sd;
    sd.pSysMem = pData;										//상수 데이터 설정.
    sd.SysMemPitch = 0;
    sd.SysMemSlicePitch = 0;
    */

    //상수 버퍼 생성.
    hr = pDev->CreateBuffer(&bd, nullptr, &pCB);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    //외부로 전달.
    *ppCB = pCB;

    return hr;
}

//초기 값도 같이 넣을 때
HRESULT CreateDynamicConstantBuffer(ID3D11Device* pDev, UINT size, LPVOID pData, ID3D11Buffer** ppCB)
{
    HRESULT hr = S_OK;
    ID3D11Buffer* pCB = nullptr;

    DWORD sizeAligned = AlignCBSize(size);

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;				
    bd.ByteWidth = sizeAligned;						
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;				

    D3D11_SUBRESOURCE_DATA sd;
    sd.pSysMem = pData;							
    sd.SysMemPitch = 0;
    sd.SysMemSlicePitch = 0;

    hr = pDev->CreateBuffer(&bd, &sd, &pCB);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    *ppCB = pCB;

    return S_OK;
}

HRESULT UpdateDynamicBuffer(ID3D11DeviceContext* pDXDC, ID3D11Resource* pBuff, LPVOID pData, UINT size)
{
    HRESULT hr = S_OK;

    //DWORD sizeAligned = AlignCBSize(size);


    D3D11_MAPPED_SUBRESOURCE mr = {};
    mr.pData = nullptr;							

    //버퍼 버퍼 접근
    hr = pDXDC->Map(pBuff, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    memcpy(mr.pData, pData, size);				
    pDXDC->Unmap(pBuff, 0);			

    return S_OK;
}

float GetEngineTime()
{
    static ULONGLONG oldtime = GetTickCount64();
    ULONGLONG 		 nowtime = GetTickCount64();
    float dTime = (nowtime - oldtime) * 0.001f;
    oldtime = nowtime;

    return dTime;
}

int DepthStencilStateCreate()
{
    D3D11_DEPTH_STENCIL_DESC  ds;
    ds.DepthEnable = TRUE;
    ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    ds.DepthFunc = D3D11_COMPARISON_LESS;
    ds.StencilEnable = FALSE;
    ds.DepthEnable = TRUE;				
    ds.StencilEnable = FALSE;				
    g_pDevice->CreateDepthStencilState(&ds, g_DSState[DS::ON].GetAddressOf());

    ds.DepthEnable = FALSE;
    g_pDevice->CreateDepthStencilState(&ds, g_DSState[DS::OFF].GetAddressOf());

    return S_OK;
}

HRESULT CreateDeviceSwapChain(HWND hWnd)
{
    HRESULT hr = S_OK;
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr, 0,
        D3D11_SDK_VERSION,
        &sd,
        g_pSwapChain.GetAddressOf(),
        g_pDevice.GetAddressOf(),
        nullptr,
        g_pDXDC.GetAddressOf()
    );

    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    return hr;
}

HRESULT CreateRenderTarget()
{
    HRESULT hr = S_OK;

    ComPtr<ID3D11Texture2D> backBuffer;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));

    if (FAILED(hr))	
    {
        ERROR_MSG(hr);
        return hr;
    }


    hr = g_pDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, g_pRTView.GetAddressOf());

    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    backBuffer->Release();

    return hr;
}

HRESULT CreateDepthStencil(int width, int height)
{
    HRESULT hr = S_OK;

    D3D11_TEXTURE2D_DESC   td = {};
    td.Width                    = width;
    td.Height                   = height;
    td.MipLevels                = 1;
    td.ArraySize                = 1;
    td.Format                   = DXGI_FORMAT_D32_FLOAT;
    td.SampleDesc.Count         = g_dwAA;
    td.SampleDesc.Quality       = 0;
    td.Usage                    = D3D11_USAGE_DEFAULT;
    td.BindFlags                = D3D11_BIND_DEPTH_STENCIL;
    td.CPUAccessFlags           = 0;
    td.MiscFlags                = 0;
    hr = g_pDevice->CreateTexture2D(&td, NULL, &g_pDS);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC  dd = {};
    dd.Format = td.Format;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;		//AA 없음.
    //dd.ViewDimension  = D3D11_DSV_DIMENSION_TEXTURE2DMS;	//+AA 설정 "MSAA"

    dd.Texture2D.MipSlice = 0;

    hr = g_pDevice->CreateDepthStencilView(g_pDS.Get(), &dd, &g_pDSView);
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    return hr;
}

void SetViewPort(int width, int height)
{
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    g_pDXDC->RSSetViewports(1, &vp);
}

HRESULT CreateInputLayout(ID3D11Device* pDev, ID3DBlob* pVScode, ID3D11InputLayout** ppLayout)
{
    HRESULT hr = S_OK;

    // 정점 입력구조 Input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,       0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,          0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };
    UINT numElements = ARRAYSIZE(layout);

    // 정접 입력구조 객체 생성 Create the input layout
    hr = g_pDevice->CreateInputLayout(layout,
        numElements,
        pVScode->GetBufferPointer(),
        pVScode->GetBufferSize(),
        ppLayout
    );
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
        return hr;
    }

    return hr;


    return E_NOTIMPL;
}

int ShaderLoad()
{
    return 0;
}

HRESULT Compile(const WCHAR* FileName, const char* EntryPoint, const char* ShaderModel, ID3DBlob** ppCode)
{
    HRESULT hr = S_OK;
    ID3DBlob* pError = nullptr;

    //컴파일 옵션1.
    UINT Flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;		//열우선 행렬 처리. 구형 DX9 이전까지의 전통적인 방식. 속도가 요구된다면, "행우선" 으로 처리할 것.
    //UINT Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;	//행우선 행렬 처리. 열 우선 처리보다 속도의 향상이 있지만, 행렬을 전치한수 GPU 에 공급해야 한다.
    //UINT Flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#ifdef _DEBUG
    Flags |= D3DCOMPILE_DEBUG;							//디버깅 모드시 옵션 추가.
#endif

    //셰이더 소스 컴파일.
    hr = D3DCompileFromFile(FileName,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,          //#include 도 컴파일
        EntryPoint,
        ShaderModel,
        Flags,						//컴파일 옵션.1
        0,							//컴파일 옵션2,  Effect 파일 컴파일시 적용됨. 이외에는 무시됨.
        ppCode,						//[출력] 컴파일된 셰이더 코드.
        &pError						//[출력] 컴파일 에러 코드.
    );
    if (FAILED(hr))
    {
        ERROR_MSG(hr);
    }

    SafeRelease(pError);
    return hr;
}



HRESULT ShaderLoad(const TCHAR* filename, ID3D11VertexShader** ppVS, ID3D11PixelShader** ppPS, ID3DBlob** ppShaderCode)
{
    HRESULT hr = S_OK;


    ID3D11VertexShader* pVS = nullptr;
    ID3DBlob* pVSCode = nullptr;
    hr = Compile((const WCHAR*)filename, "VS_Main", "vs_5_0", &pVSCode);
    if (FAILED(hr))
    {
        MessageBox(NULL, _T("[실패] ShaderLoad :: Vertex Shader 컴파일 실패"), _T("Error"), MB_OK | MB_ICONERROR);
        return hr;
    }

    hr = g_pDevice->CreateVertexShader(pVSCode->GetBufferPointer(), pVSCode->GetBufferSize(), nullptr, &pVS);
    if (FAILED(hr))
    {
        SafeRelease(pVSCode);
        return hr;
    }

    ID3D11PixelShader* pPS = nullptr;
    ID3DBlob* pPSCode = nullptr;
    hr = Compile((const WCHAR*)filename, "PS_Main", "ps_5_0", &pPSCode);
    if (FAILED(hr))
    {
        MessageBox(NULL, _T("[실패] ShaderLoad :: Pixel Shader 컴파일 실패"), _T("Error"), MB_OK | MB_ICONERROR);
        return hr;
    }

    hr = g_pDevice->CreatePixelShader(pPSCode->GetBufferPointer(), pPSCode->GetBufferSize(), nullptr, &pPS);
    if (FAILED(hr))
    {
        SafeRelease(pVSCode);
        SafeRelease(pPSCode);
        return hr;
    }
    SafeRelease(pPSCode);

    *ppVS = pVS;
    *ppPS = pPS;
    *ppShaderCode = pVSCode;

    return hr;

}

HRESULT LoadVertexShader(const TCHAR* filename, ID3D11VertexShader** ppVS, ID3DBlob** ppVSCode)
{
    HRESULT hr = S_OK;

    ID3D11VertexShader* pVS = nullptr;
    ID3DBlob* pCode = nullptr;

    hr = Compile((const WCHAR*)filename, "VS_Main", "vs_5_0", &pCode);
    if (FAILED(hr))
    {
        MessageBox(NULL, _T("[실패] Vertex Shader 컴파일 실패"), _T("Error"), MB_OK | MB_ICONERROR);
        return hr;
    }

    hr = g_pDevice->CreateVertexShader(
        pCode->GetBufferPointer(),
        pCode->GetBufferSize(),
        nullptr,
        &pVS);

    if (FAILED(hr))
    {
        SafeRelease(pCode);
        return hr;
    }

    *ppVS = pVS;
    *ppVSCode = pCode;

    return hr;
}

HRESULT LoadPixelShader(const TCHAR* filename, ID3D11PixelShader** ppPS)
{
    HRESULT hr = S_OK;

    ID3D11PixelShader* pPS = nullptr;
    ID3DBlob* pCode = nullptr;

    hr = Compile((const WCHAR*)filename, "PS_Main", "ps_5_0", &pCode);
    if (FAILED(hr))
    {
        MessageBox(NULL, _T("[실패] Pixel Shader 컴파일 실패"), _T("Error"), MB_OK | MB_ICONERROR);
        return hr;
    }

    hr = g_pDevice->CreatePixelShader(
        pCode->GetBufferPointer(),
        pCode->GetBufferSize(),
        nullptr,
        &pPS);

    SafeRelease(pCode);

    if (FAILED(hr))
        return hr;

    *ppPS = pPS;
    return hr;
}

//렌더 타겟용 텍스쳐 만들기
HRESULT RTTexCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex)
{
    //텍스처 정보 구성.
    D3D11_TEXTURE2D_DESC td = {};
    //ZeroMemory(&td, sizeof(td));
    td.Width = width;						//텍스처크기(1:1)
    td.Height = height;
    td.MipLevels = 0;
    td.ArraySize = 1;
    td.Format = fmt;							//텍스처 포멧 (DXGI_FORMAT_R8G8B8A8_UNORM 등..)
    td.SampleDesc.Count = 1;					// AA 없음.
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;		//용도 : RT + SRV
    td.CPUAccessFlags = 0;
    td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    //텍스처 생성.
    ID3D11Texture2D* pTex = NULL;
    HRESULT hr = g_pDevice->CreateTexture2D(&td, NULL, &pTex);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateTexture2D 실패"));
        return hr;
    }

    //성공후 외부로 리턴.
    if (ppTex) *ppTex = pTex;

    return hr;
}

//렌더 타겟용 뷰 만들기
//그림 그릴 추가 렌더 타겟
HRESULT RTViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView)
{
    //렌더타겟 정보 구성.
    D3D11_RENDER_TARGET_VIEW_DESC rd = {};
    //ZeroMemory(&rd, sizeof(rd));
    rd.Format = fmt;										//텍스처와 동일포멧유지.
    rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;		//2D RT.
    rd.Texture2D.MipSlice = 0;								//2D RT 용 추가 설정 : 밉멥 분할용 밉멥레벨 인덱스.
    //rd.Texture2DMS.UnusedField_NothingToDefine = 0;		//2D RT + AA 용 추가 설정

    //렌더타겟 생성.
    ID3D11RenderTargetView* pRTView = NULL;
    HRESULT hr = g_pDevice->CreateRenderTargetView(pTex, &rd, &pRTView);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateRenderTargetView 실패"));
        return hr;
    }



    //성공후 외부로 리턴.
    if (ppRTView) *ppRTView = pRTView;

    return hr;
}


//렌더타겟용 쉐이더 리소스 뷰
//렌더 타겟에 그려진 그림을 리소스용으로 만들기
HRESULT RTSRViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV)
{
    //셰이더리소스뷰 정보 구성.
    D3D11_SHADER_RESOURCE_VIEW_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Format = fmt;										//텍스처와 동일포멧유지.
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;		//2D SRV.
    sd.Texture2D.MipLevels = -1;								//2D SRV 추가 설정 : 밉멥 설정.
    sd.Texture2D.MostDetailedMip = 0;
    //sd.Texture2DMS.UnusedField_NothingToDefine = 0;		//2D SRV+AA 추가 설정

    //셰이더리소스뷰 생성.
    ID3D11ShaderResourceView* pTexRV = NULL;
    HRESULT hr = g_pDevice->CreateShaderResourceView(pTex, &sd, &pTexRV);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateShaderResourceView 실패"));
        return hr;
    }

    //성공후 외부로 리턴.
    if (ppTexRV) *ppTexRV = pTexRV;

    return hr;
}


HRESULT RTCubeTexCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex)
{
    //텍스처 정보 구성.
    D3D11_TEXTURE2D_DESC td = {};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 0;
    td.ArraySize = 6;							//ArraySize추가로 해당 텍스쳐가 6면을 가진다는 것을 의미
    td.Format = fmt;							//텍스처 포멧 (DXGI_FORMAT_R8G8B8A8_UNORM 등..)
    td.SampleDesc.Count = 1;					// AA 없음.
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    td.CPUAccessFlags = 0;
    td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;	//밉맵 플래그, 텍스쳐 큐브 추가

    //텍스처 생성.
    ID3D11Texture2D* pTex = NULL;
    HRESULT hr = g_pDevice->CreateTexture2D(&td, NULL, &pTex);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateTexture2D 실패"));
        return hr;
    }

    //성공후 외부로 리턴.
    if (ppTex) *ppTex = pTex;

    return hr;
}

HRESULT CubeRTViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView, UINT faceIndex)
{
    //렌더타겟 정보 구성.
    D3D11_RENDER_TARGET_VIEW_DESC rd = {};
    //ZeroMemory(&rd, sizeof(rd));
    rd.Format = fmt;
    rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;		//렌더 타겟 뷰가 Texture2D를 배열로 가지고 있음
    rd.Texture2D.MipSlice = 0;
    //rd.Texture2DMS.UnusedField_NothingToDefine = 0;		
    rd.Texture2DArray.FirstArraySlice = faceIndex;				//렌더 타겟 뷰에서 이미지 배열 몇번 째에 저장할 것인지(예상)
    rd.Texture2DArray.ArraySize = 1;							//렌더 타겟 뷰의 배열 크기를 1
    //6이 아닌 이유는 렌더타겟 6개를 만들고 렌더 타겟에 그려진 이미지를 하나의 이미지에
    //데이터를 저장하기 때문에(예상)

//렌더타겟 생성.
    ID3D11RenderTargetView* pRTView = NULL;
    HRESULT hr = g_pDevice->CreateRenderTargetView(pTex, &rd, &pRTView);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateRenderTargetView 실패"));
        return hr;
    }



    //성공후 외부로 리턴.
    if (ppRTView) *ppRTView = pRTView;

    return hr;

}

//큐브 렌더타겟 리소스 뷰 생성
HRESULT RTCubeSRViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV)
{
    //셰이더리소스뷰 정보 구성.
    D3D11_SHADER_RESOURCE_VIEW_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Format = fmt;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;		//이 리소스는 큐브 텍스쳐임을 나타냄
    sd.Texture2D.MipLevels = -1;
    sd.Texture2D.MostDetailedMip = 0;
    //sd.Texture2DMS.UnusedField_NothingToDefine = 0;		

    //셰이더리소스뷰 생성.
    ID3D11ShaderResourceView* pTexRV = NULL;
    HRESULT hr = g_pDevice->CreateShaderResourceView(pTex, &sd, &pTexRV);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateShaderResourceView 실패"));
        return hr;
    }

    //성공후 외부로 리턴.
    if (ppTexRV) *ppTexRV = pTexRV;

    return hr;

}


//뎁스 스텐실 생성
HRESULT DSCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D*& pDSTex, ID3D11DepthStencilView*& pDSView)
{
    HRESULT hr = S_OK;


    //---------------------------------- 
    // 깊이/스텐실 버퍼용 빈 텍스처로 만들기.	
    //---------------------------------- 
    //깊이/스텐실 버퍼 정보 구성.
    D3D11_TEXTURE2D_DESC td = {};
    //ZeroMemory(&td, sizeof(td));
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = fmt;									//원본 RT 와 동일 포멧유지.
    //td.Format  = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;	//깊이 버퍼 (32bit) + 스텐실 (8bit) / 신형 하드웨어 (DX11)
    td.SampleDesc.Count = 1;							// AA 없음.
    //td.SampleDesc.Count = g_dwAA;						// AA 설정 - RT 과 동일 규격 준수.
    //td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;			//깊이-스텐실 버퍼용으로 설정.
    td.MiscFlags = 0;
    //깊이/스텐실 버퍼용 빈 텍스처로 만들기.	
    hr = g_pDevice->CreateTexture2D(&td, NULL, &pDSTex);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] DS / CreateTexture 실패"));
        return hr;
    }


    //---------------------------------- 
    // 깊이/스텐실 뷰 생성.
    //---------------------------------- 
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    //ZeroMemory(&dd, sizeof(dd));
    dd.Format = td.Format;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;			//2D (AA 없음)
    //dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;			//2D (AA 적용)
    dd.Texture2D.MipSlice = 0;
    //깊이/스텐실 뷰 생성.
    hr = g_pDevice->CreateDepthStencilView(pDSTex, &dd, &pDSView);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] DS / CreateDepthStencilView 실패"));
        return hr;
    }

    return hr;
}

void RasterStateCreate()
{
    //[상태객체 1] 기본 렌더링 상태 개체.
    D3D11_RASTERIZER_DESC rd;
    rd.FillMode = D3D11_FILL_SOLID;		//삼각형 색상 채우기.(기본값)
    rd.CullMode = D3D11_CULL_NONE;		//컬링 없음. (기본값은 컬링 Back)		
    rd.FrontCounterClockwise = false;   //이하 기본값...
    rd.DepthBias = 0;
    rd.DepthBiasClamp = 0;
    rd.SlopeScaledDepthBias = 0;
    rd.DepthClipEnable = true;
    rd.ScissorEnable = false;
    rd.MultisampleEnable = false;
    rd.AntialiasedLineEnable = false;
    //레스터라이져 상태 객체 생성.
    g_pDevice->CreateRasterizerState(&rd, g_RState[RS::SOLID].GetAddressOf());


    //[상태객체2] 와이어 프레임 그리기. 
    rd.FillMode = D3D11_FILL_WIREFRAME;
    rd.CullMode = D3D11_CULL_NONE;
    //레스터라이져 객체 생성.
    g_pDevice->CreateRasterizerState(&rd, g_RState[RS::WIREFRM].GetAddressOf());

    //[상태객체3] 컬링 On! "CCW"
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_BACK;
    g_pDevice->CreateRasterizerState(&rd, g_RState[RS::CULLBACK].GetAddressOf());

    //[상태객체4] 와이어 프레임 + 컬링 On! "CCW"
    rd.FillMode = D3D11_FILL_WIREFRAME;
    rd.CullMode = D3D11_CULL_BACK;
    g_pDevice->CreateRasterizerState(&rd, g_RState[RS::WIRECULLBACK].GetAddressOf());

}


