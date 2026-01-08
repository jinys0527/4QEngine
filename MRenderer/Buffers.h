#pragma once
#include "DX11.h"
#include <vector>

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
	BaseConstBuffer				BCBuffer;
	ComPtr<ID3D11Buffer>		pBCB;

	std::vector<ComPtr<ID3D11Buffer>>* vertexBuffers = nullptr;
	std::vector<ComPtr<ID3D11Buffer>>* indexBuffers	 = nullptr;
	std::vector<UINT32>* indexcounts				 = nullptr;
};

