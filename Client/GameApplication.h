#pragma once
 
#include "NzWndBase.h"
#include <wrl/client.h>
#include "RenderData.h"
#include "Engine.h"
#include "RenderTargetContext.h"
#include <string>
#include <unordered_map>

class ServiceRegistry;
class Engine;
class Renderer;
class SceneManager;
class GameObject;
class InputManager;
class AssetLoader;
class SoundManager;


class GameApplication : public NzWndBase
{

public:
	GameApplication(ServiceRegistry& serviceRegistry, Engine& engine, Renderer& renderer, SceneManager& sceneManager) : NzWndBase(), m_Services(serviceRegistry), m_Engine(engine), m_Renderer(renderer), m_SceneManager(sceneManager) { }
	virtual ~GameApplication() = default;

	bool Initialize();
	void Run();
	void Finalize();

	bool OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
	void UpdateInput();
	void UpdateLogic();
	void Update();
	void ApplySceneBGM();
	void Render();
	
	void OnResize(int width, int height) override;
	void OnClose() override;

	//GameObject* m_Player;
	//GameObject* m_Obstacle;



	float m_fFrameCount;
	float m_GameSpeed = 1.0f;

	ServiceRegistry& m_Services;
	Engine&			 m_Engine;
	Renderer&		 m_Renderer;
	SceneManager&	 m_SceneManager;
	InputManager&    m_InputManager;

	AssetLoader* m_AssetLoader;
	SoundManager* m_SoundManager;
};

