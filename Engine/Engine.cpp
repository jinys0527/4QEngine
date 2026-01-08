#include "pch.h"
#include "Engine.h"

void Engine::Initailize()
{
	m_EventDispatcher = std::make_unique<EventDispatcher>();
	m_InputManager    = std::make_unique<InputManager>(*m_EventDispatcher);
	m_Device		  =	std::make_unique<Device>();
	
	//m_AssetManager = std::make_unique<AssetManager>(*m_Renderer);
	//m_SoundAssetManager = std::make_unique<SoundAssetManager>();

	m_GameTimer.Reset();
}

void Engine::UpdateTime()
{
	m_GameTimer.Tick();
}

void Engine::UpdateInput()
{
	m_InputManager->Update();
}

//아직 생성은 안함
void Engine::CreateDevice(HWND hwnd) {
	if (!m_Device) {
		// 없다면 생성 시도
		m_Device = std::make_unique<Device>();
	}
	//실제 Device class 객체 생성
	m_Device->Create(hwnd);
}