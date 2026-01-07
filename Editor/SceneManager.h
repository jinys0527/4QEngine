//editor
#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "Scene.h"
#include "EventDispatcher.h"
#include "SoundManager.h"
#include "UIManager.h"


// editor 용으로 수정 필요
class SceneManager
{
	friend class Editor;
public:
	SceneManager(/*D2DRenderer& renderer, */EventDispatcher& eventDispatcher,
		/*AssetManager& assetManager, SoundAssetManager& soundAssetManager,*/
		SoundManager& soundManager, GameManager& gameManager, UIManager& uiManager) : 
		/*m_Renderer(renderer), */
		m_EventDispatcher(eventDispatcher), /*
		m_AssetManager(assetManager), m_SoundAssetManager(soundAssetManager), */
		m_SoundManager(soundManager), 
		m_GameManager(gameManager), 
		m_UIManager(uiManager) { }
	~SceneManager() = default;

	void Initialize();
	void Update(float deltaTime);
	void Render();

	//void SetCamera(CameraObject* camera) { m_Camera = camera; }
	//CameraObject* GetCamera() { return m_Camera; }

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
	std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
	std::shared_ptr<Scene> m_CurrentScene;
	//CameraObject* m_Camera = nullptr;
	//D2DRenderer& m_Renderer;
	//AssetManager& m_AssetManager;
	//SoundAssetManager& m_SoundAssetManager;

	EventDispatcher&  m_EventDispatcher;
	SoundManager&     m_SoundManager;
	GameManager&      m_GameManager;
	UIManager&	      m_UIManager;
	bool			  m_ShouldQuit;

	std::string m_ChangeSceneName;
};