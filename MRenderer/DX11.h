#pragma once
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D11")		

#include <Windows.h>
#include <d3d11.h>
#include <tchar.h>
#include <wrl/client.h>
#include <memory>
//#include "DXMath.h"
#include "DirectXMath.h"

using Microsoft::WRL::ComPtr;

using namespace DirectX;

typedef DirectX::XMFLOAT4 COLOR;
typedef DXGI_MODE_DESC DISPLAY;

//#ifndef KEY_UP
//#define KEY_UP(key)	    ((GetAsyncKeyState(key)&0x8001) == 0x8001)
//#define KEY_DOWN(key)	((GetAsyncKeyState(key)&0x8000) == 0x8000)
//#define IsKeyUp         KEY_UP
//#define IsKeyDown       KEY_DOWN
//enum VK_CHAR
//{
//	VK_0 = 0x30,
//	VK_1, VK_2, VK_3, VK_4, VK_5, VK_6, VK_7, VK_8, VK_9,
//
//	VK_A = 0x41,
//	VK_B, VK_C, VK_D, VK_E, VK_F, VK_G, VK_H, VK_I, VK_J, VK_K, VK_L, VK_M, VK_N,
//	VK_O, VK_P, VK_Q, VK_R, VK_S, VK_T, VK_U, VK_V, VK_W, VK_X, VK_Y, VK_Z,
//};
//#endif

#define ERROR_MSG(hr) \
    { \
        wchar_t buf[512]; \
        swprintf_s(buf, L"[실패] \n파일: %s\n줄: %d\nHRESULT: 0x%08X", \
                   _CRT_WIDE(__FILE__), __LINE__, hr); \
        MessageBox(NULL, buf, L"Error", MB_OK | MB_ICONERROR); \
    }

enum class DS{
	ON,				//깊이버퍼 ON! (기본값), 스텐실버퍼 OFF.
	OFF,				//깊이버퍼 OFF!
	//DS_DEPTH_WRITE_OFF,			//깊이버퍼 쓰기 끄기

	MAX,
};

//렌더링 상태 객체들 : 엔진 전체 공유함. "Device.cpp"
extern ComPtr<ID3D11DepthStencilState> g_DSState[static_cast<int>(DS::MAX)];


extern ComPtr<ID3D11Device> g_pDevice;
extern ComPtr<ID3D11DeviceContext> g_pDXDC;
extern ComPtr<IDXGISwapChain> g_pSwapChain;
extern ComPtr<ID3D11RenderTargetView> g_pRTView; 
extern	BOOL 		g_bVSync;
extern int g_MonitorWidth;
extern int g_MonitorHeight;


//bool DXSetup(HWND hWnd);
bool DXSetup(HWND hWnd, int width, int height);
void Draw();

int		ClearBackBuffer(COLOR col);
int		ClearBackBuffer(UINT flag, COLOR col, float depth = 1.0f, UINT stencil = 0);
int     Flip();


void	GetDeviceInfo();
HRESULT GetAdapterInfo(DXGI_ADAPTER_DESC1* pAd);
void	SystemUpdate(float dTime);



#pragma region 버퍼 운용함수
int 	CreateBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppBuff);
int 	UpdateBuffer(ID3D11Buffer* pBuff, LPVOID pData, UINT size);

int		CreateVertexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, UINT stride, ID3D11Buffer** ppVB);
int		CreateIndexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, ID3D11Buffer** ppIB);
int		CreateConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB);
//ID3D11Buffer*	CreateConstantBuffer(UINT size);

HRESULT CreateDynamicConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB);
HRESULT CreateDynamicConstantBuffer(ID3D11Device* pDev, UINT size, LPVOID pData, ID3D11Buffer** ppCB);
HRESULT UpdateDynamicBuffer(ID3D11DeviceContext* pDXDC, ID3D11Resource* pBuff, LPVOID pData, UINT size);

#pragma endregion

DWORD	AlignCBSize(DWORD size);

int	CreateInputLayout(ID3D11Device* pDev, D3D11_INPUT_ELEMENT_DESC* ed, DWORD num, ID3DBlob* pVSCode, ID3D11InputLayout** ppLayout);


float GetEngineTime();

template <typename T>
void SafeRelease(T*& ptr)
{
	if (ptr)
	{
		ptr->Release();
		ptr = nullptr;
	}
}

void SetViewPort(int width, int height);
