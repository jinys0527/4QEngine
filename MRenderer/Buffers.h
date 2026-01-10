#pragma once
#include "DX11.h"
#include <vector>

//기본 상수 버퍼
struct BaseConstBuffer
{
	XMFLOAT4X4 mWorld = XMFLOAT4X4{};
	XMFLOAT4X4 mView  = XMFLOAT4X4{};
	XMFLOAT4X4 mProj  = XMFLOAT4X4{};
	XMFLOAT4X4 mVP    = XMFLOAT4X4{};
	//XMFLOAT4X4 mWVP;		추후에 추가. 버텍스가 많아지면

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
	ComPtr<ID3D11Device>            pDevice;
	ComPtr<ID3D11DeviceContext>     pDXDC;
	ComPtr<ID3D11RenderTargetView>	pRTView;
	ComPtr<ID3D11DepthStencilView>  pDSView;


	EnumArray<ComPtr<ID3D11DepthStencilState>, static_cast<size_t>(DS::MAX_)>	DSState;
	EnumArray<ComPtr<ID3D11RasterizerState>, static_cast<size_t>(RS::MAX_)>		RState;

	BaseConstBuffer				BCBuffer;
	ComPtr<ID3D11Buffer>		pBCB;			//GPU에 넘기는 버퍼
	SkinningConstBuffer			SkinCBuffer;
	ComPtr<ID3D11Buffer>		pSkinCB;

	std::vector<ComPtr<ID3D11Buffer>>* vertexBuffers = nullptr;
	std::vector<ComPtr<ID3D11Buffer>>* indexBuffers = nullptr;
	std::vector<UINT32>* indexcounts = nullptr;

	ComPtr<ID3D11InputLayout> inputLayout = nullptr;

	ComPtr<ID3D11VertexShader> VS;
	ComPtr<ID3D11PixelShader> PS;
	ComPtr<ID3DBlob> VSCode;

};