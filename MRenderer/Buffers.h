#pragma once
#include "DX11.h"
#include "RenderData.h"
#include "ResourceHandle.h"
#include <unordered_map>
#include <functional>

//기본 상수 버퍼
struct BaseConstBuffer
{
	XMFLOAT4X4		mWorld = XMFLOAT4X4{};
	XMFLOAT4X4		mWorldInvTranspose = XMFLOAT4X4{};
	XMFLOAT4X4		mTextureMask = XMFLOAT4X4{};
};

struct CameraConstBuffer
{
	XMFLOAT4X4	mView = XMFLOAT4X4{};
	XMFLOAT4X4	mProj = XMFLOAT4X4{};
	XMFLOAT4X4	mVP = XMFLOAT4X4{};
	XMFLOAT4X4  mSkyBox = XMFLOAT4X4{};
	XMFLOAT4X4  mShadow = XMFLOAT4X4{};
	XMFLOAT3	camPos = XMFLOAT3{};
	//XMFLOAT4X4 mWVP;		추후에 추가. 버텍스가 많아지면
	float		padding = 0.0f;
};

struct Light
{
	XMFLOAT4X4 mLightViewProj{};

	XMFLOAT4   Color{ 1,1,1,1 };

	XMFLOAT3   Pos{ 0,0,0 };
	float      Range = 0;

	XMFLOAT3   worldDir{ 0,-1,0 };
	float      padding0 = 0.0f;

	XMFLOAT3   viewDir{ 0,-1,0 };
	float      Intensity = 1.0f;

	float		SpotInnerAngle = 0.0f;
	float		SpotOutterAngle = 0.0f;
	float		AttenuationRadius = 0.0f;
	float		padding1 = 0.0f;

	UINT       CastShadow = TRUE;
	float      padding[3]{ 0,0,0 };
};
constexpr int MAX_LIGHTS = 16;		//★빛 개수 정해지면 변경할 것
struct LightConstBuffer
{
	Light	lights[MAX_LIGHTS];
	UINT	lightCount = 0;
	FLOAT   padding[3]{ 0.0f, 0.0f, 0.0f };
};

constexpr size_t kMaxSkinningBones = 256;

struct SkinningConstBuffer
{
	XMFLOAT4X4 bones[kMaxSkinningBones]{};
	UINT boneCount = 0;
	float padding[3]{ 0.0f, 0.0f, 0.0f };
};

struct UIBuffer
{
	XMFLOAT4X4  TextPosition{};
	XMFLOAT4	TextColor{ 1.0f, 1.0f, 1.0f, 1.0f };

	XMFLOAT4	Color{ 1.0f, 1.0f, 1.0f, 1.0f };

	FLOAT		Opacity;
	float		padding[3] = { 0, 0 };
};

struct MaterialBuffer
{
	FLOAT		saturation = 1.0f;
};

struct VertexShaderResources
{
	ComPtr<ID3D11VertexShader>	vertexShader;
	ComPtr<ID3DBlob>			vertexShaderCode;
};

struct PixelShaderResources
{
	ComPtr<ID3D11PixelShader>	pixelShader;
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
	CameraConstBuffer			CameraCBuffer;
	ComPtr<ID3D11Buffer>		pCameraCB;			
	SkinningConstBuffer			SkinCBuffer;
	ComPtr<ID3D11Buffer>		pSkinCB;
	LightConstBuffer			LightCBuffer;
	ComPtr<ID3D11Buffer>		pLightCB;
	UIBuffer					UIBuffer;
	ComPtr<ID3D11Buffer>		pUIB;
	MaterialBuffer				MatBuffer;
	ComPtr<ID3D11Buffer>		pMatB;

	std::unordered_map<MeshHandle, ComPtr<ID3D11Buffer>>*					vertexBuffers	= nullptr;
	std::unordered_map<MeshHandle, ComPtr<ID3D11Buffer>>*					indexBuffers	= nullptr;
	std::unordered_map<MeshHandle, UINT32>*									indexCounts		= nullptr;
	std::unordered_map<TextureHandle, ComPtr<ID3D11ShaderResourceView>>*	textures		= nullptr;
	std::unordered_map<VertexShaderHandle, VertexShaderResources>*		    vertexShaders = nullptr;
	std::unordered_map<PixelShaderHandle, PixelShaderResources>*		    pixelShaders = nullptr;

	ComPtr<ID3D11InputLayout> InputLayout = nullptr;
	ComPtr<ID3D11InputLayout> InputLayout_P = nullptr;

	ComPtr<ID3D11VertexShader> VS;
	ComPtr<ID3D11PixelShader> PS;
	ComPtr<ID3DBlob> VSCode;

	ComPtr<ID3D11VertexShader> VS_P;
	ComPtr<ID3D11PixelShader> PS_P;
	ComPtr<ID3D11PixelShader> PS_Frustum;
	ComPtr<ID3DBlob> VSCode_P;

	ComPtr<ID3D11VertexShader> VS_PBR;
	ComPtr<ID3D11PixelShader> PS_PBR;
	ComPtr<ID3DBlob> VSCode_PBR;

	ComPtr<ID3D11VertexShader> VS_Quad;
	ComPtr<ID3D11PixelShader> PS_Quad;
	ComPtr<ID3DBlob> VSCode_Quad;

	ComPtr<ID3D11VertexShader> VS_Post;
	ComPtr<ID3D11PixelShader> PS_Post;
	ComPtr<ID3DBlob> VSCode_Post;


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

	//PostPass용
	ComPtr<ID3D11Texture2D>				pRTScene_Post;
	ComPtr<ID3D11ShaderResourceView>	pTexRvScene_Post;
	ComPtr<ID3D11RenderTargetView>		pRTView_Post;

	//BlurPass용
	ComPtr<ID3D11Texture2D>				pRTScene_Blur;
	ComPtr<ID3D11ShaderResourceView>	pTexRvScene_Blur;
	ComPtr<ID3D11RenderTargetView>		pRTView_Blur;



	std::function<void()> DrawFullscreenQuad;
	std::function<void()> DrawGrid;
	std::function<void(const RenderData::FrameData& f)> UpdateGrid;

	//블러 테스트
	ComPtr<ID3D11ShaderResourceView> Vignetting;

	//스카이박스 테스트
	ComPtr<ID3D11ShaderResourceView>	SkyBox;
	ComPtr<ID3D11VertexShader>			VS_SkyBox;
	ComPtr<ID3D11PixelShader>			PS_SkyBox;

	//그림자 매핑 테스트
	ComPtr<ID3D11VertexShader>			VS_Shadow;
	ComPtr<ID3D11PixelShader>			PS_Shadow;

	//FullScreenTriangle
	ComPtr<ID3D11VertexShader>			VS_FSTriangle;
	std::function<void()>				DrawFSTriangle;

	//텍스트 그리기
	std::function<void(float width, float height)> MyDrawText;
};