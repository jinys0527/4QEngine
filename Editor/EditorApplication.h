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
#include <filesystem>
#include <array>

class GameObject;

class EditorApplication : public NzWndBase
{

public:
	EditorApplication(Engine& engine,Renderer& renderer, SceneManager& sceneManager, SoundManager& soundManager, AssetLoader& assetLoader) 
		: NzWndBase(), m_Engine(engine),m_Renderer(renderer), m_SceneManager(sceneManager), m_SoundManager(soundManager), m_AssetLoader(assetLoader) {
	}

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
	void UpdateEditorCamera();

	//Render 관련
	void Render();
	void RenderImGUI();
	void RenderSceneView();

	// 필요한 창 생성
	void DrawMainMenuBar();
	void DrawHierarchy();
	void DrawInspector();
	void DrawFolderView();
	void DrawResourceBrowser();

	//Gui 관련
	void CreateDockSpace();
	void SetupEditorDockLayout();



	void OnResize(int width, int height) override;
	void OnClose() override;

	float m_fFrameCount = 0.0f;
	UINT64 m_FrameIndex = 0;

	Engine& m_Engine;
	SceneManager& m_SceneManager;
	SoundManager& m_SoundManager;
	AssetLoader&  m_AssetLoader;
	Renderer& m_Renderer;
	RenderData::FrameData m_FrameData;
	RenderTargetContext m_SceneRenderTarget;
	RenderTargetContext m_SceneRenderTarget_edit;
	EditorViewport m_Viewport;

	// Hier
	std::string m_SelectedObjectName;

	//이름 변경 관련
	std::string m_LastSelectedObjectName;
	std::string m_LastSceneName;
	std::array<char, 256> m_ObjectNameBuffer;
	std::array<char, 256> m_SceneNameBuffer;

	// Floder View 변수
	// resource root 지정 // 추후 수정 필요 //작업 환경마다 다를 수 있음
	std::filesystem::path m_ResourceRoot = "../Resources/Scenes"; 
	std::filesystem::path m_SelectedResourcePath;

	std::filesystem::path m_CurrentScenePath;

	std::filesystem::path m_PendingDeletePath;
	std::filesystem::path m_PendingSavePath;

	bool m_OpenSaveConfirm = false;
	bool m_OpenDeleteConfirm = false;
};