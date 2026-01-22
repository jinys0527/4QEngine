// Game / Client 용
#pragma once
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <string>
#include "Scene.h"
#include "EventDispatcher.h"
#include "IEventListener.h"


class ServiceRegistry;
class GameManager;
class UIManager;
class CameraObject;
class InputManager; 


class SceneManager : public IEventListener
{
	friend class Editor;
public:
	SceneManager(ServiceRegistry& serviceRegistry) : m_Services(serviceRegistry) { }
	~SceneManager() = default;

	void Initialize();
	void Update(float deltaTime);
	void StateUpdate(float deltaTime);
	void Render();

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

	void SetChangeScene(const std::string& name);
	void SetEventDispatcher(EventDispatcher* eventDispatcher);
	void OnEvent(EventType type, const void* data) override;

private:
	ServiceRegistry& m_Services;
	void LoadGameScenesFromDirectory(const std::filesystem::path& directoryPath, const std::vector<std::string>& sceneNames);
	bool LoadGameSceneFromJson(const std::filesystem::path& filepath);

	std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
	std::shared_ptr<Scene> m_CurrentScene;
	CameraObject*   m_Camera = nullptr;
	GameManager*	m_GameManager;
	UIManager*		m_UIManager;
	InputManager*	m_InputManager;
	EventDispatcher* m_EventDispatcher = nullptr;
	std::filesystem::path scenesPath = "../Resources/Scenes"; //Resource 파일 경로
	bool m_ShouldQuit = false;

	std::string m_ChangeSceneName = "";
};

