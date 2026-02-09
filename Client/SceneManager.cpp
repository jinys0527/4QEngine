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
#include "GameDataRepository.h"
#include "InitiativeUIComponent.h"

void SceneManager::Initialize()
{
	
	m_UIManager    = &m_Services.Get<UIManager>();
	m_GameManager  = &m_Services.Get<GameManager>();
	m_InputManager = &m_Services.Get<InputManager>();
	if (m_GameManager)
	{
		m_GameManager->SetServices(&m_Services);
		DataSheetPaths dataPaths{};
		dataPaths.itemsPath = "Data/items.csv";
		dataPaths.enemiesPath = "Data/enemies.csv";
		dataPaths.dropTablesPath = "Data/drop_tables.csv";
		m_GameManager->SetDataSheetPaths(dataPaths);
		m_GameManager->SetFloorSceneNames({ "Stage1_Test ", "Stage2_Test ", "Stage3 ", "Ending " });
	}
	if (m_InputManager)
	{
		m_InputManager->SetGameManager(m_GameManager);
		m_InputManager->SetEventDispatcher(m_EventDispatcher);
	}
	// Sound Manager

	//std::filesystem::path scenesPath = "../Resources/Scenes";

	// Game에서 로드할 것 여기서 명시 
	LoadGameScenesFromDirectory(scenesPath,{
		"Stage1_Test",
		"Stage2_Test"
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

	if (m_GameManager)
	{
		m_GameManager->Update(deltaTime);
	}

	if (m_UIManager)
	{
		m_UIManager->Update(deltaTime);
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
	if (m_UIManager)
		m_UIManager->Reset();
	SetEventDispatcher(nullptr);
	m_Scenes.clear();
	m_CurrentScene.reset();
}

void SceneManager::Render(RenderData::FrameData& frameData)
{
	if (!m_CurrentScene)
	{
		return;
	}

	m_CurrentScene->Render(frameData);
	if (m_UIManager)
	{
		m_UIManager->BuildUIFrameData(frameData);
	}

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
		if (m_UIManager && m_CurrentScene)
		{
			auto& uiMap = m_UIManager->GetUIObjects();
			auto itScene = uiMap.find(m_CurrentScene->GetName());
			if (itScene != uiMap.end())
			{
				for (const auto& [uiName, uiObject] : itScene->second)
				{
					if (!uiObject)
					{
						continue;
					}

					if (auto* initiative = uiObject->GetComponent<InitiativeUIComponent>())
					{
						initiative->DetachFromDispatcher();
					}
					uiObject->SetScene(nullptr);
				}
			}
		}

		if (m_GameManager && m_CurrentScene)
		{
			m_GameManager->CapturePlayerData(m_CurrentScene.get());
		}
		m_CurrentScene = it->second;
		m_CurrentScene->Enter();

		m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());

		m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());

		if (m_GameManager)
		{
			m_CurrentScene->SetGameManager(m_GameManager);
			m_InputManager->SetGameManager(m_GameManager);
			m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
			m_GameManager->SetActiveScene(m_CurrentScene.get());
			m_GameManager->ApplyPlayerData(m_CurrentScene.get());
			m_GameManager->TurnReset();
		}
		 
 		if (m_UIManager)
		{
 			m_UIManager->SetCurrentScene(name);
			RestoreSceneUI(m_CurrentScene);
 		}
	}
}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() const
{
	return m_CurrentScene;
}

void SceneManager::ChangeScene(const std::string& name)
{

	if (m_CurrentScene) {
		if (m_UIManager)
		{
			auto& uiMap = m_UIManager->GetUIObjects();
			auto itScene = uiMap.find(m_CurrentScene->GetName());
			if (itScene != uiMap.end())
			{
				for (const auto& [uiName, uiObject] : itScene->second)
				{
					if (!uiObject)
					{
						continue;
					}

					if (auto* initiative = uiObject->GetComponent<InitiativeUIComponent>())
					{
						initiative->DetachFromDispatcher();
					}
					uiObject->SetScene(nullptr);
				}
			}
			m_UIManager->ClearSceneUI(m_CurrentScene->GetName());
		}

		if (m_GameManager)
		{
			m_GameManager->CapturePlayerData(m_CurrentScene.get());
		}
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
		if (m_UIManager)
		{
			m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		}

		if (m_GameManager)
		{
			m_CurrentScene->SetGameManager(m_GameManager);
			m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
			m_GameManager->ApplyPlayerData(m_CurrentScene.get());
		}

		if (m_UIManager)
		{
			m_UIManager->SetCurrentScene(name);
			RestoreSceneUI(m_CurrentScene);
		}
	}
}

void SceneManager::ChangeScene()
{
	// 현재 이름과 다르면 Change 
	if (m_ChangeSceneName.empty() || !m_CurrentScene || m_ChangeSceneName == m_CurrentScene->GetName()) {
		return;
	}
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
	ChangeScene();
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

	if (m_UIManager && j.contains("ui"))
	{
		m_SceneUIData[loadedScene->GetName()] = j.at("ui");
		m_UIManager->SetEventDispatcher(&loadedScene->GetEventDispatcher());
		m_UIManager->DeserializeSceneUI(loadedScene->GetName(), j.at("ui"));
		auto& uiMap = m_UIManager->GetUIObjects();
		auto itScene = uiMap.find(loadedScene->GetName());
		if (itScene != uiMap.end())
		{
			for (const auto& [name, uiObject] : itScene->second)
			{
				if (uiObject)
				{
					uiObject->SetScene(loadedScene.get());
					uiObject->Start();
				}
			}
		}
	}

	return true;

}

void SceneManager::RestoreSceneUI(const std::shared_ptr<Scene>& scene)
{
	if (!m_UIManager || !scene)
	{
		return;
	}

	const auto& sceneName = scene->GetName();
	auto itData = m_SceneUIData.find(sceneName);
	if (itData == m_SceneUIData.end())
	{
		return;
	}

	m_UIManager->DeserializeSceneUI(sceneName, itData->second);
	auto& uiMap = m_UIManager->GetUIObjects();
	auto itScene = uiMap.find(sceneName);
	if (itScene != uiMap.end())
	{
		for (const auto& [name, uiObject] : itScene->second)
		{
			if (uiObject)
			{
				uiObject->SetScene(scene.get());
				uiObject->Start();
			}
		}
	}
}
