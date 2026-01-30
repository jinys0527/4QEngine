// Game / Client 용
#include "pch.h"
#include "SceneManager.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include "Scene.h"
#include "UIManager.h"
#include "ClientScene.h"
#include "DefaultScene.h"
#include "InputManager.h"
#include "Event.h"
#include "json.hpp"
#include "CameraObject.h"


void SceneManager::Initialize()
{
	
	m_UIManager    = &m_Services.Get<UIManager>();
	m_GameManager  = &m_Services.Get<GameManager>();
	m_InputManager = &m_Services.Get<InputManager>();
	// Sound Manager

	//std::filesystem::path scenesPath = "../Resources/Scenes";

	// Game에서 로드할 것 여기서 명시 
	LoadGameScenesFromDirectory(scenesPath,{
		"TitleScene",
		"Stage1 ",
		"Stage2 ",
		"Stage3 ",
		"Ending ",// 제일 처음 실행될 Scene
		//"BossStage"
		});

	//Load 실패
	if (!m_CurrentScene)
	{
		std::cerr << "No scene loaded from " << scenesPath.string() << std::endl;
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

	if (totalTime >= 0.016f) {
		m_CurrentScene->FixedUpdate();
	}
	
	m_CurrentScene->Update(deltaTime);
}

void SceneManager::StateUpdate(float deltaTime)
{
	if (!m_CurrentScene)
		return;

	m_CurrentScene->StateUpdate(deltaTime);
}

void SceneManager::Reset()
{
	if (m_GameManager)
		m_GameManager->ClearEventDispatcher();
	SetEventDispatcher(nullptr);
	m_Scenes.clear();
	m_CurrentScene.reset();
}

void SceneManager::Render()
{
	if (!m_CurrentScene)
	{
		return;
	}

	RenderData::FrameData frameData{};
	m_CurrentScene->Render(frameData);

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
		/*m_Camera = m_CurrentScene->GetGameCamera().get();
		m_InputManager->SetViewportRect({ 0, 0, static_cast<LONG>(m_Camera->GetViewportSize().Width), static_cast<LONG>(m_Camera->GetViewportSize().Height) });*/
		m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());

		if (m_GameManager)
		{
			m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
		}
// 
// 		if (m_UIManager) {
// 			m_UIManager->SetCurrentScene(name);
// 		}
	}
}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() const
{
	return m_CurrentScene;
}

void SceneManager::ChangeScene(const std::string& name)
{

	if (m_CurrentScene) {
		m_CurrentScene->Leave();
	}


	auto it = m_Scenes.find(name);

	if (it != m_Scenes.end())
	{
		m_CurrentScene = it->second;
		m_CurrentScene->Enter();
		if (m_InputManager)
		{
			m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		}
		SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
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
	// 현재 이름과 다르면 Change 
	if (m_ChangeSceneName != m_CurrentScene->GetName()) {
		ChangeScene(m_ChangeSceneName);
		m_ChangeSceneName.clear();
	}
}

void SceneManager::SetChangeScene(const std::string& name)
{
	m_ChangeSceneName = name;
}

void SceneManager::SetEventDispatcher(EventDispatcher* eventDispatcher)
{
	if (m_EventDispatcher == eventDispatcher)
	{
		return;
	}

	if (m_EventDispatcher)
	{
		m_EventDispatcher->RemoveListener(EventType::SceneChangeRequested, this);
	}

	m_EventDispatcher = eventDispatcher;

	if (m_EventDispatcher)
	{
		m_EventDispatcher->AddListener(EventType::SceneChangeRequested, this);
	}
}

void SceneManager::OnEvent(EventType type, const void* data)
{
	if (type != EventType::SceneChangeRequested || !data)
	{
		return;
	}

	const auto* request = static_cast<const Events::SceneChangeRequest*>(data);
	if (!request)
	{
		return;
	}

	SetChangeScene(request->name);
}


void SceneManager::LoadGameScenesFromDirectory(const std::filesystem::path& directoryPath, const std::vector<std::string>& sceneNames)
{

	if (directoryPath.empty() || !std::filesystem::exists(directoryPath))
	{
		std::cout << "Invalid directory" << std::endl;
		return;
	}

	for (const auto& sceneName : sceneNames)
	{
		std::filesystem::path scenePath =
			directoryPath / (sceneName + ".json");

		if (!std::filesystem::exists(scenePath))
		{
			std::cout << "Scene not found: " << sceneName << std::endl;
			continue;
		}

		std::cout << scenePath << " Scene Find" << std::endl;
		LoadGameSceneFromJson(scenePath);
	}

	// 첫 번째 Scene을 Entry Scene으로 설정
	if (!m_CurrentScene && !sceneNames.empty())
	{
		SetCurrentScene(sceneNames.front());
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

	auto loadedScene = std::make_shared<ClientScene>(m_Services);
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
