#pragma once
#include "GameTimer.h"
#include <memory>
#include "EventDispatcher.h"
#include "InputManager.h"
//#include "D2DRenderer.h"
//#include "AssetManager.h"
//#include "SoundAssetManager.h"

class Engine
{
public:
	Engine() { Initailize(); }
	~Engine() = default;
	
	void UpdateTime();
	void UpdateInput();

	//D2DRenderer& GetRenderer() { return *m_Renderer; }
	EventDispatcher& GetEventDispatcher() { return *m_EventDispatcher; }
	//AssetManager& GetAssetManager() { return *m_AssetManager; }
	InputManager& GetInputManager() { return *m_InputManager; }
	//SoundAssetManager& GetSoundAssetManager() { return *m_SoundAssetManager; }

	GameTimer& GetTimer() { return m_GameTimer; }

	void Reset()
	{
		//m_SoundAssetManager.reset();
		//m_AssetManager.reset();
	//	m_Renderer.reset();
		m_InputManager.reset();
		m_EventDispatcher.reset();
	}

private:
	void Initailize();

private:
	GameTimer m_GameTimer;
	std::unique_ptr<EventDispatcher> m_EventDispatcher = nullptr;
	std::unique_ptr<InputManager>    m_InputManager    = nullptr;
	//std::unique_ptr<D2DRenderer> m_Renderer = nullptr;
	//std::unique_ptr<AssetManager> m_AssetManager = nullptr;
	//std::unique_ptr<SoundAssetManager> m_SoundAssetManager = nullptr;
};

