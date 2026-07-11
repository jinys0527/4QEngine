// Game / Client 용
#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "Scene.h"
#include "EventDispatcher.h"
#include "IEventListener.h"

class ServiceRegistry;
class GameManager;
class UIManager;
class CameraObject;

class SceneManager
{
	friend class Editor;
public:
	SceneManager(ServiceRegistry& serviceRegistry) : m_Services(serviceRegistry) { }
	~SceneManager() = default;

	void Initialize();
	void Update(float deltaTime);
	void StateUpdate(float deltaTime);
	void Render(RenderData::FrameData& frameData);

	void SetCamera(CameraObject* camera) { m_Camera = camera; }
	CameraObject* GetCamera() { return m_Camera; }

	std::shared_ptr<Scene> AddScene(const std::string& name, std::shared_ptr<Scene> scene);
	void SetCurrentScene(const std::string& name);
	std::shared_ptr<Scene> GetCurrentScene() const;

	void ChangeScene(const std::string& name);

	void ChangeScene();

	void Reset()
	{
		SetEventDispatcher(nullptr);
		m_Scenes.clear();
		m_CurrentScene.reset();
	}

	void RequestQuit() { m_ShouldQuit = true; }
	bool ShouldQuit() const { return m_ShouldQuit; }

	void SetChangeScene(std::string name);

private:
	enum class SceneTransitionPhase
	{
		None,
		FadeOut,
		FadeIn,
	};

	ServiceRegistry& m_Services;
	void LoadGameScenesFromDirectory(const std::filesystem::path& directoryPath, const std::vector<std::string>& sceneNames);
	bool LoadGameSceneFromJson(const std::filesystem::path& filepath);

	std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
	std::unordered_map<std::string, nlohmann::json> m_SceneUIData;
	std::unordered_map<std::string, nlohmann::json> m_SceneTemplateData;
	std::shared_ptr<Scene> m_CurrentScene;
	CameraObject*   m_Camera = nullptr;
	GameManager*	m_GameManager;
	UIManager*		m_UIManager;
	
	bool m_ShouldQuit = false;

	std::string m_ChangeSceneName = "";
};

