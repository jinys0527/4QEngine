#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "Scene.h"

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
		m_Scenes.clear();
		m_CurrentScene.reset();
	}

	void RequestQuit() { m_ShouldQuit = true; }
	bool ShouldQuit() const { return m_ShouldQuit; }

	void SetChangeScene(std::string name);

private:
	ServiceRegistry& m_Services;

	std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
	std::shared_ptr<Scene> m_CurrentScene;
	CameraObject*   m_Camera = nullptr;
	GameManager*	m_GameManager;
	UIManager*		m_UIManager;
	
	bool m_ShouldQuit = false;

	std::string m_ChangeSceneName;
};

