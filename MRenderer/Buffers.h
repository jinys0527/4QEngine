#pragma once
#include "DX11.h"
#include <unordered_map>

//기본 상수 버퍼
struct BaseConstBuffer
{
	XMFLOAT4X4 mWorld = XMFLOAT4X4{};
	XMFLOAT4X4 mView = XMFLOAT4X4{};
	XMFLOAT4X4 mProj = XMFLOAT4X4{};
	XMFLOAT4X4 mVP = XMFLOAT4X4{};
	//XMFLOAT4X4 mWVP;		추후에 추가. 버텍스가 많아지면

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

	std::unordered_map<UINT, ComPtr<ID3D11Buffer>>* vertexBuffers = nullptr;
	std::unordered_map<UINT, ComPtr<ID3D11Buffer>>* indexBuffers = nullptr;
	std::unordered_map<UINT, UINT32>* indexcounts = nullptr;


	ComPtr<ID3D11InputLayout> inputLayout = nullptr;

	ComPtr<ID3D11VertexShader> VS;
	ComPtr<ID3D11PixelShader> PS;
	ComPtr<ID3DBlob> VSCode;

	ComPtr<ID3D11InputLayout> InputLayout_P;
	ComPtr<ID3D11VertexShader> VS_P;
	ComPtr<ID3DBlob> VSCode_P;


	//그림자 매핑용
	ComPtr<ID3D11Texture2D>				pDSTex_Shadow;
	ComPtr<ID3D11DepthStencilView>		pDSViewScene_Shadow;
	TextureSize							ShadowTextureSize = { 0,0 };

	//DepthPass용
	ComPtr<ID3D11Texture2D>				pDSTex_Depth;
	ComPtr<ID3D11DepthStencilView>		pDSViewScene_Depth;

};