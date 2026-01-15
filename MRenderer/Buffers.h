#pragma once
#include "DX11.h"
#include "RenderData.h"
#include "ResourceHandle.h"
#include <unordered_map>
#include <functional>

//기본 상수 버퍼
struct BaseConstBuffer
{
	XMFLOAT4X4 mWorld = XMFLOAT4X4{};
	XMFLOAT4X4 mView  = XMFLOAT4X4{};
	XMFLOAT4X4 mProj  = XMFLOAT4X4{};
	XMFLOAT4X4 mVP    = XMFLOAT4X4{};
	//XMFLOAT4X4 mWVP;		추후에 추가. 버텍스가 많아지면

};

struct Light
{
	XMFLOAT3   vPos{ 0.0f, 0.0f, 0.0f };
	FLOAT      Range = 0.0f;
	XMFLOAT3   vDir{ 0.0f, -1.0f, 0.0f };
	FLOAT      SpotAngle = 0.0f;
	XMFLOAT3   Color{ 1.0f, 1.0f, 1.0f };
	FLOAT      Intensity = 1.0f;
	XMFLOAT4X4 mLightViewProj{};
	UINT       CastShadow = TRUE;
	FLOAT      padding[3]{ 0.0f, 0.0f, 0.0f };
};

constexpr int MAX_LIGHTS = 16;		//★빛 개수 정해지면 변경할 것
struct LightConstBuffer
{
	Light	lights[MAX_LIGHTS];
	UINT	lightCount;
	FLOAT   padding[3]{ 0.0f, 0.0f, 0.0f };
};

constexpr size_t kMaxSkinningBones = 128;

struct SkinningConstBuffer
{
	XMFLOAT4X4 bones[kMaxSkinningBones]{};
	UINT boneCount = 0;
	float padding[3]{ 0.0f, 0.0f, 0.0f };
};


struct RenderContext
{
	TextureSize						WindowSize = { 0,0 };

	ComPtr<ID3D11Device>            pDevice;
	ComPtr<ID3D11DeviceContext>     pDXDC;
	ComPtr<ID3D11RenderTargetView>	pRTView;		//메인 렌더 타겟
	ComPtr<ID3D11DepthStencilView>  pDSView;		//메인 뎁스 스텐실뷰


	EnumArray<ComPtr<ID3D11DepthStencilState>, static_cast<size_t>(DS::MAX_)>	DSState;
	EnumArray<ComPtr<ID3D11RasterizerState>, static_cast<size_t>(RS::MAX_)>		RState;
	EnumArray<ComPtr<ID3D11SamplerState>, static_cast<size_t>(SS::MAX_)>		SState;			//샘플러 상태
	EnumArray<ComPtr<ID3D11BlendState>, static_cast<size_t>(BS::MAX_)>			BState;			//블렌딩 상태

	BaseConstBuffer				BCBuffer;
	ComPtr<ID3D11Buffer>		pBCB;			//GPU에 넘기는 버퍼
	SkinningConstBuffer			SkinCBuffer;
	ComPtr<ID3D11Buffer>		pSkinCB;
	LightConstBuffer			LightCBuffer;
	ComPtr<ID3D11Buffer>		pLightCB;

	std::unordered_map<MeshHandle, ComPtr<ID3D11Buffer>>*					vertexBuffers	= nullptr;
	std::unordered_map<MeshHandle, ComPtr<ID3D11Buffer>>*					indexBuffers	= nullptr;
	std::unordered_map<MeshHandle, UINT32>*									indexCounts		= nullptr;
	std::unordered_map<TextureHandle, ComPtr<ID3D11ShaderResourceView>>*	textures		= nullptr;

	ComPtr<ID3D11InputLayout> inputLayout = nullptr;

	ComPtr<ID3D11VertexShader> VS;
	ComPtr<ID3D11PixelShader> PS;
	ComPtr<ID3DBlob> VSCode;

	ComPtr<ID3D11InputLayout> InputLayout_P;
	ComPtr<ID3D11VertexShader> VS_P;
	ComPtr<ID3DBlob> VSCode_P;


	//imgui용 == Scene Draw용
	bool isEditCam = false;
	ComPtr<ID3D11Texture2D>				pRTScene_Imgui;
	ComPtr<ID3D11ShaderResourceView>	pTexRvScene_Imgui;
	ComPtr<ID3D11RenderTargetView>		pRTView_Imgui;

	ComPtr<ID3D11Texture2D>				pDSTex_Imgui;
	ComPtr<ID3D11DepthStencilView>		pDSViewScene_Imgui;

	ComPtr<ID3D11Texture2D>				pRTScene_Imgui_edit;
	ComPtr<ID3D11ShaderResourceView>	pTexRvScene_Imgui_edit;
	ComPtr<ID3D11RenderTargetView>		pRTView_Imgui_edit;

	ComPtr<ID3D11Texture2D>				pDSTex_Imgui_edit;
	ComPtr<ID3D11DepthStencilView>		pDSViewScene_Imgui_edit;

	//그림자 매핑용
	ComPtr<ID3D11Texture2D>				pDSTex_Shadow;
	ComPtr<ID3D11DepthStencilView>		pDSViewScene_Shadow;
	ComPtr<ID3D11ShaderResourceView>	pShadowRV;
	TextureSize							ShadowTextureSize = { 0,0 };

	//DepthPass용
	ComPtr<ID3D11Texture2D>				pDSTex_Depth;
	ComPtr<ID3D11DepthStencilView>		pDSViewScene_Depth;
	ComPtr<ID3D11ShaderResourceView>	pDepthRV;

	std::function<void()> DrawFullscreenQuad;
	std::function<void()> DrawGrid;
	std::function<void(const RenderData::FrameData& f)> UpdateGrid;

};