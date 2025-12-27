#pragma once
 
#include "NzWndBase.h"
#include <wrl/client.h>
#include "Engine.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "Editor.h"

class GameObject;
class CameraObject;

class GameApplication : public NzWndBase
{

public:
#ifdef _EDITOR
	GameApplication(Engine& engine, SceneManager& sceneManager, SoundManager& soundManager, Editor& editor) : NzWndBase(), m_Engine(engine), m_SceneManager(sceneManager), m_SoundManager(soundManager), m_Editor(editor) {}
#else
	GameApplication(Engine& engine, SceneManager& sceneManager, SoundManager& soundManager) : NzWndBase(), m_Engine(engine), m_SceneManager(sceneManager), m_SoundManager(soundManager) { }
#endif
	virtual ~GameApplication() = default;

	bool Initialize();
	void Run();
	void Finalize();

	bool OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
	void UpdateInput();
	void UpdateLogic();
	void Update();

	void Render();
	void RenderImGUI();
	
	void OnResize(int width, int height) override;
	void OnClose() override;

	//GameObject* m_Player;
	//GameObject* m_Obstacle;


#ifdef _EDITOR
	Editor& m_Editor;
#endif

	float m_fFrameCount;

	Engine& m_Engine;
	SceneManager& m_SceneManager;
	SoundManager& m_SoundManager;
};

