#pragma once

#include "NzWndBase.h"
#include <wrl/client.h>
#include "Engine.h"
#include "SceneManager.h"
#include "SoundManager.h"

class GameObject;

class EditorApplication : public NzWndBase
{

public:
	EditorApplication(Engine& engine, SceneManager& sceneManager, SoundManager& soundManager) : NzWndBase(), m_Engine(engine), m_SceneManager(sceneManager), m_SoundManager(soundManager) {}

	virtual ~EditorApplication() = default;

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
	float m_fFrameCount;
	Engine& m_Engine;
	SceneManager& m_SceneManager;
	SoundManager& m_SoundManager;
};