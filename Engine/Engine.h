#pragma once
#include "GameTimer.h"
#include <memory>
#include "EventDispatcher.h"
#include "InputManager.h"
#include "Device.h"
//#include "D2DRenderer.h"
//#include "AssetManager.h"
//#include "SoundAssetManager.h"

class Engine
{
public:
	Engine() { Initailize(); }
	~Engine() = default;
	
	void UpdateTime();
	float GetTime() const;
	void UpdateInput();
	void CreateDevice(HWND hwnd);

	EventDispatcher& GetEventDispatcher()	 { return *m_EventDispatcher; }
	InputManager&	 GetInputManager()		 { return *m_InputManager; }
	Device&			 GetDevice()			 { return *m_Device; }
	
	// 개별 Device, DXDC Get 
	ID3D11Device*			 Get3DDevice() const	  { return m_Device ? m_Device->GetDevice().Get() : nullptr; }
	ID3D11DeviceContext*	 GetD3DDXDC()  const	  { return m_Device ? m_Device->GetDXDC().Get() : nullptr; }

	//SoundAssetManager& GetSoundAssetManager() { return *m_SoundAssetManager; }
	//AssetManager& GetAssetManager() { return *m_AssetManager; }
	//D2DRenderer& GetRenderer() { return *m_Renderer; }

	GameTimer& GetTimer() { return m_GameTimer; }

	void Reset()
	{
		//m_SoundAssetManager.reset();
		//m_AssetManager.reset();
		//	m_Renderer.reset();
		m_InputManager.reset();
		m_EventDispatcher.reset();
		m_Device.reset();
	}

private:
	void Initailize();

private:
	GameTimer m_GameTimer;
	std::unique_ptr<EventDispatcher> m_EventDispatcher = nullptr;
	std::unique_ptr<InputManager>    m_InputManager    = nullptr;

	std::unique_ptr<Device>			 m_Device		   = nullptr; 	//Device 에 member로 Device, DXDC 존재

	//std::unique_ptr<D2DRenderer> m_Renderer = nullptr;
	//std::unique_ptr<AssetManager> m_AssetManager = nullptr;
	//std::unique_ptr<SoundAssetManager> m_SoundAssetManager = nullptr;
};

