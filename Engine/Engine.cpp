#include "pch.h"
#include "Engine.h"

void Engine::Initailize()
{
	m_EventDispatcher = std::make_unique<EventDispatcher>();
	m_InputManager = std::make_unique<InputManager>(*m_EventDispatcher);
	//m_Renderer = std::make_unique<D2DRenderer>();
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