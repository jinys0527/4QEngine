//editor
#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "Scene.h"

class ServiceRegistry;
class UIManager;
class InputManager;

class CameraObject;
// editor 용으로 수정 필요
class SceneManager
{
	friend class Editor;
public:
	SceneManager(ServiceRegistry& serviceRegistry) : 
		m_Services(serviceRegistry),
		m_ShouldQuit(false) { }

	~SceneManager() = default;

	void Initialize();
	void Update(float deltaTime);
	void StateUpdate(float deltaTime);
	void Render();

	void SetCamera(std::shared_ptr<CameraObject> camera) { m_Camera = camera; }
	std::shared_ptr<CameraObject> GetCamera() { return m_Camera; }
	//std::shared_ptr<Scene> AddScene(const std::string& name, std::shared_ptr<Scene> scene);
	void SetCurrentScene(std::shared_ptr<Scene> scene);
	std::shared_ptr<Scene> GetCurrentScene() const;
	void ChangeScene(const std::string& name);
	void ChangeScene();
	bool CreateNewScene(const std::filesystem::path& filePath);
	bool LoadSceneFromJson(const std::filesystem::path& filePath);
	bool SaveSceneToJson(const std::filesystem::path& filePath) const;

	void Reset()
	{
		//m_Scenes.clear();
		m_CurrentScene.reset();
	}

	void RequestQuit() { m_ShouldQuit = true; }
	bool ShouldQuit() const { return m_ShouldQuit; }

	void SetChangeScene(std::string name);

private:
	ServiceRegistry& m_Services;

	//std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
	std::shared_ptr<Scene> m_CurrentScene;
	std::shared_ptr<CameraObject> m_Camera = nullptr;
	
	InputManager*		  m_InputManager;
	UIManager*			  m_UIManager;
	std::filesystem::path m_CurrentScenePath;
	bool				  m_ShouldQuit;
	std::string			  m_ChangeSceneName;
};