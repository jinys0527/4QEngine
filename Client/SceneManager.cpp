// Game / Client 용
#include "pch.h"
#include "SceneManager.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include "UIManager.h"
#include "DefaultScene.h"
#include "InputManager.h"
#include "json.hpp"
#include <fstream>


void SceneManager::Initialize()
{
	/*m_UIManager.Start();
	m_UIManager.SetCurrentScene("TitleScene");*/
	m_UIManager = &m_Services.Get<UIManager>();
	m_GameManager = &m_Services.Get<GameManager>();
	m_InputManager = &m_Services.Get<InputManager>();

	std::filesystem::path scenesPath = std::filesystem::path("Resources") / "Scenes";
	LoadGameScenesFromDirectory(scenesPath);
	if (!m_CurrentScene)
	{
		std::filesystem::path fallbackPath = std::filesystem::path("Client") / "GameScene.json";
		LoadGameSceneFromJson(fallbackPath);
	}
}

void SceneManager::Update(float deltaTime)
{
	if (!m_CurrentScene)
		return;
	if (m_CurrentScene->GetIsPause())
		deltaTime = 0.0f;

	static float totalTime = 0;
	totalTime += deltaTime;

	if (totalTime >= 0.016f)
		m_CurrentScene->FixedUpdate();

	m_CurrentScene->Update(deltaTime);
}

void SceneManager::StateUpdate(float deltaTime)
{
	if (!m_CurrentScene)
		return;

	m_CurrentScene->StateUpdate(deltaTime);
}

void SceneManager::Render()
{
	if (!m_CurrentScene)
	{
		return;
	}

	RenderData::FrameData frameData{};
	m_CurrentScene->Render(frameData);
	//m_Renderer.Draw(frameData);
	/*std::vector<RenderInfo> renderInfo;
	std::vector<UIRenderInfo> uiRenderInfo;
	std::vector<UITextInfo> uiTextInfo;
	m_CurrentScene->Render(renderInfo, uiRenderInfo, uiTextInfo);
	m_Renderer.Draw(renderInfo, uiRenderInfo, uiTextInfo);*/
}

std::shared_ptr<Scene> SceneManager::AddScene(const std::string& name, std::shared_ptr<Scene> scene)
{
	m_Scenes[name] = scene;

	m_Scenes[name]->SetGameManager(&m_Services.Get<GameManager>());
	m_Scenes[name]->SetSceneManager(this);

	return m_Scenes[name];
}

void SceneManager::SetCurrentScene(const std::string& name)
{
	auto it = m_Scenes.find(name);
	if (it != m_Scenes.end())
	{
		m_CurrentScene = it->second;
		m_CurrentScene->Enter();


		if (m_InputManager)
		{
			m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		}
		if (m_UIManager)
		{
			//UI 생기면 그때
			//m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
			//m_UIManager->SetCurrentScene(name);
		}
		if (m_GameManager)
		{
			m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
		}

		if (m_UIManager) {
			m_UIManager->SetCurrentScene(name);
		}
	}
}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() const
{
	return m_CurrentScene;
}

void SceneManager::ChangeScene(const std::string& name)
{
	if(m_CurrentScene)
		m_CurrentScene->Leave();

	auto it = m_Scenes.find(name);

	if (it != m_Scenes.end())
	{
		m_CurrentScene = it->second;
		m_CurrentScene->Enter();
		if (m_InputManager)
		{
			m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		}

		//UI 생기면 그때
		//if (m_UIManager)
		//{
		//	m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		//}

		if (m_GameManager)
		{
			m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
		}

		if (m_UIManager)
		{
			m_UIManager->SetCurrentScene(name);
		}
	}
}

void SceneManager::ChangeScene()
{
	ChangeScene(m_ChangeSceneName);
}

void SceneManager::SetChangeScene(std::string name)
{
	m_ChangeSceneName = name;
}

void SceneManager::LoadGameScenesFromDirectory(const std::filesystem::path& directoryPath)
{
	std::cout << "Testing" << std::endl;
	if (directoryPath.empty() || !std::filesystem::exists(directoryPath))
	{
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const auto& path = entry.path();
		if (path.extension() != ".json")
		{
			continue;
		}

		LoadGameSceneFromJson(path);
	}

	if (!m_CurrentScene && !m_Scenes.empty())
	{
		SetCurrentScene(m_Scenes.begin()->first);
	}
}

bool SceneManager::LoadGameSceneFromJson(const std::filesystem::path& filepath)
{
	if (filepath.empty()) {
		std::cout << "No Scene Name"<<std::endl;
		return false;
	}

	std::ifstream ifs(filepath);

	if (!ifs.is_open()) {
		return false;
	}

	nlohmann::json j;
	ifs >> j;

	auto loadedScene = std::make_shared<Scene>(m_Services);
	loadedScene->SetName(filepath.stem().string());
	loadedScene->Initialize();
	loadedScene->Deserialize(j);
	loadedScene->SetIsPause(false);
	AddScene(loadedScene->GetName(), loadedScene);
	if (!m_CurrentScene)
	{
		SetCurrentScene(loadedScene->GetName());
	}

	return true;

}
