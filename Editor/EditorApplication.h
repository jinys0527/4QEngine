#pragma once

#include "NzWndBase.h"
#include <memory>
#include <string>
#include "EditorViewport.h"
#include "Engine.h"
#include "RenderTargetContext.h" 
#include "RenderData.h"
#include "json.hpp"
#include <filesystem>
#include <array>
#include "UndoManager.h"

class ServiceRegistry;
class Renderer;
class SceneManager;
class GameObject;
class AssetLoader;
class SoundManager;


enum class EditorPlayState
{
	Stop,
	Play,
	Pause
};


class EditorApplication : public NzWndBase
{

public:
	EditorApplication(ServiceRegistry& serviceRegistry, Engine& engine,Renderer& renderer, SceneManager& sceneManager) 
		: NzWndBase(), m_Services(serviceRegistry), m_Engine(engine),m_Renderer(renderer), m_SceneManager(sceneManager)
		, m_EditorViewport("Editor")
		, m_GameViewport("Game"){
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
	void DrawGizmo();

	void FocusEditorCameraOnObject(const std::shared_ptr<GameObject>& object);

	void DrawPlayPauseButton();
	//Gui 관련
	void CreateDockSpace();
	void SetupEditorDockLayout();



	void OnResize(int width, int height) override;
	void OnClose() override;

	float m_fFrameCount = 0.0f;
	UINT64 m_FrameIndex = 0;

	ServiceRegistry&      m_Services;
	Engine&			      m_Engine;
	SceneManager&         m_SceneManager;
	Renderer&             m_Renderer;
	AssetLoader*		  m_AssetLoader;
	SoundManager*		  m_SoundManager;
	InputManager*	      m_InputManager;
	RenderData::FrameData m_FrameData;
	RenderTargetContext   m_SceneRenderTarget;
	RenderTargetContext   m_SceneRenderTarget_edit;

	EditorViewport        m_EditorViewport;
	EditorViewport        m_GameViewport;
	EditorPlayState       m_EditorState = EditorPlayState::Stop;
	// Hier
	std::string m_SelectedObjectName;

	//이름 변경 관련
	std::string m_LastSelectedObjectName;
	std::string m_LastSceneName;
	std::array<char, 256> m_ObjectNameBuffer;
	std::array<char, 256> m_SceneNameBuffer;

	nlohmann::json m_ObjectClipboard;
	bool m_ObjectClipboardHasData = false;
	bool m_ObjectClipboardIsOpaque = true;

	// Floder View 변수
	// resource root 지정 // 추후 수정 필요 //작업 환경마다 다를 수 있음
	std::filesystem::path m_ResourceRoot = "../Resources/Scenes"; 
	std::filesystem::path m_SelectedResourcePath;

	std::filesystem::path m_CurrentScenePath;

	std::filesystem::path m_PendingDeletePath;
	std::filesystem::path m_PendingSavePath;

	bool m_OpenSaveConfirm = false;
	bool m_OpenDeleteConfirm = false;

	UndoManager m_UndoManager;
};