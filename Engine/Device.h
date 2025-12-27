#pragma once
class Device
{
public:
	void Create(HWND hwnd);

	ComPtr<ID3D11Device>		 GetDevice() { return m_Device; }
	ComPtr<ID3D11DeviceContext>  GetDC    () { return m_DXDC;   }
private:
	ComPtr<ID3D11Device>		 m_Device;
	ComPtr<ID3D11DeviceContext>  m_DXDC;
	D3D_FEATURE_LEVEL m_FeatureLevels = D3D_FEATURE_LEVEL_11_0;
};



