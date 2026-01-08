#pragma once

#include "NzWndBase.h"
#include <memory>
#include <string>
#include "AssetLoader.h"
#include "EditorViewport.h"
#include "Engine.h"
#include "RenderTargetContext.h" 
#include "Renderer.h"
#include "SceneManager.h"
#include "SoundManager.h"

class GameObject;

class EditorApplication : public NzWndBase
{

public:
	EditorApplication(Engine& engine,Renderer& renderer, SceneManager& sceneManager, SoundManager& soundManager) 
		: NzWndBase(), m_Engine(engine),m_Renderer(renderer), m_SceneManager(sceneManager), m_SoundManager(soundManager) {}

	virtual ~EditorApplication() = default;

	bool Initialize();
	void Run();
	void Finalize();

	bool OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
	void UpdateInput();
	//void UpdateLogic();
	void Update();
	void UpdateSceneViewport();
	void Render();
	void RenderImGUI();
	void RenderSceneView();
	void DrawHierarchy();
	void DrawInspector();



	void OnResize(int width, int height) override;
	void OnClose() override;

	float m_fFrameCount = 0.0f;
	UINT64 m_FrameIndex = 0;

	Engine& m_Engine;

	SceneManager& m_SceneManager;
	SoundManager& m_SoundManager;
	//AssetLoader m_AssetLoader;
	Renderer& m_Renderer;
	RenderData::FrameData m_FrameData;
	RenderTargetContext m_SceneRenderTarget;
	EditorViewport m_Viewport;


	std::string m_SelectedObjectName;
};