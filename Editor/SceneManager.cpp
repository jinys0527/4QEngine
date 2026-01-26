//editor
#include "pch.h"
#include "SceneManager.h"
#include "DefaultScene.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include "Renderer.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include "InputManager.h"
#include "UIManager.h"
#include "CameraObject.h"

//editor 용으로 개발 필요함 - 편집할 Scene 선택, 생성
void SceneManager::Initialize()
{
	/*m_UIManager.Start();
	m_UIManager.SetCurrentScene("TitleScene");*/

	// 씬없는 경우.. 인데
	// ★★★★★★★★★★★★★★
	// Editor의 경우 m_Scenes 로 여러개를 들고 있을 필요가 없음. -> 이제 있음 Play모드 시 게임 흐름 보려면 
	// 자기가 쓰는 Scene 하나만 있으면됨.
	// 다른 Scene의 경우 경로에서 Scenedata json으로 띄우기만 하면 됨.
	// 그냥 Default 생성
	m_InputManager = &m_Services.Get<InputManager>();
	m_UIManager = &m_Services.Get<UIManager>();
	m_GameManager = &m_Services.Get<GameManager>();

	auto emptyScene = std::make_shared<DefaultScene>(m_Services);
	emptyScene->SetSceneManager(this);
	emptyScene->SetName("Untitled Scene");
	emptyScene->Initialize();
	emptyScene->SetIsPause(true);
	emptyScene->Enter();
	// editor는 Current Scene만 띄운다.
	SetCurrentScene(emptyScene);
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
	/*std::vector<RenderInfo> renderInfo;
	std::vector<UIRenderInfo> uiRenderInfo;
	std::vector<UITextInfo> uiTextInfo;
	m_CurrentScene->Render(renderInfo, uiRenderInfo, uiTextInfo);
	m_Renderer.Draw(renderInfo, uiRenderInfo, uiTextInfo);*/
}


void SceneManager::SetCurrentScene(std::shared_ptr<Scene> scene)
{
	auto oldScene = m_CurrentScene;

	m_CurrentScene = scene;
	m_CurrentScene->Enter();
	m_Camera = m_CurrentScene->GetGameCamera();
	m_InputManager->SetViewportRect({ 0, 0, static_cast<LONG>(m_Camera->GetViewportSize().Width), static_cast<LONG>(m_Camera->GetViewportSize().Height) });

	m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
	m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
	SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());

	if (m_GameManager)
	{
		m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
	}

}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() const
{
	return m_CurrentScene;
}

void SceneManager::ChangeScene(const std::string& name)
{
	if (name.empty())
	{
		return;
	}

	if (!m_CurrentScene)
	{
		return;
	}

	if (m_CurrentScene->GetName() == name)
	{
		return;
	}

	const auto* scenePath = FindScenePathByName(name);
	if (!scenePath)
	{
		return;
	}
	const bool wasPaused = m_CurrentScene->GetIsPause();
	m_CurrentScene->Leave();
	if (LoadSceneFromJson(*scenePath))
	{
		m_CurrentScene->SetIsPause(wasPaused);
	}
}

void SceneManager::ChangeScene()
{
	if (!m_CurrentScene)
	{
		return;
	}

	if (m_ChangeSceneName != m_CurrentScene->GetName())
	{
		ChangeScene(m_ChangeSceneName);
		m_ChangeSceneName.clear();
	}

}

void SceneManager::SetChangeScene(std::string name)
{
	m_ChangeSceneName = name;
}


const std::filesystem::path* SceneManager::FindScenePathByName(const std::string& name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	auto it = m_Scenes.find(name);
	if (it == m_Scenes.end())
	{
		return nullptr;
	}
	return &it->second;
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

bool SceneManager::CreateNewScene(const std::filesystem::path& filePath)
{
	if (filePath.empty())
	{
		return false;
	}

	auto newScene = std::make_shared<DefaultScene>(m_Services);
	newScene->SetSceneManager(this);
	newScene->SetName(filePath.stem().string());
	newScene->Initialize();
	newScene->SetIsPause(true);
	//AddOrReplaceScene(newScene);
	SetCurrentScene(newScene);
	m_CurrentScenePath = filePath;

	return SaveSceneToJson(filePath);
}


bool SceneManager::LoadSceneFromJson(const std::filesystem::path& filePath)
{	// 선택했을때 해당 Scene을 Deserialize 해서 
	// 현재 씬으로
	if (filePath.empty())
	{
		return false;
	}

	std::ifstream ifs(filePath);
	if (!ifs.is_open())
	{
		return false;
	}

	nlohmann::json j;
	ifs >> j;

	auto loadedScene = std::make_shared<DefaultScene>(m_Services);
	loadedScene->SetName(filePath.stem().string());
	loadedScene->Initialize();
	loadedScene->Deserialize(j);
	loadedScene->SetIsPause(true);
	//AddOrReplaceScene(loadedScene);
	SetCurrentScene(loadedScene);
	m_CurrentScenePath = filePath;

	return true;
}

bool SceneManager::LoadSceneFromJsonData(const nlohmann::json& data, const std::filesystem::path& filePath)
{
	if(data.is_null())
		return false;

	auto loadedScene = std::make_shared<DefaultScene>(m_Services);
	if (!filePath.empty())
	{
		loadedScene->SetName(filePath.stem().string());
	}
	else
	{
		loadedScene->SetName("Untitled Scene");
	}

	loadedScene->Initialize();
	loadedScene->Deserialize(data);
	loadedScene->SetIsPause(true);
	//AddOrReplaceScene(loadedScene);
	SetCurrentScene(loadedScene);
	m_CurrentScenePath = filePath;

	return true;
}

bool SceneManager::SaveSceneToJson(const std::filesystem::path& filePath)const
{	
	// 같은 이름이 있는경우 그냥 덮어쓰기 할것
	if (!m_CurrentScene)
	{
		return false;
	}

	std::ofstream ofs(filePath);
	if (!ofs.is_open())
	{
		return false;
	}

	nlohmann::json j;
	m_CurrentScene->Serialize(j);
	ofs << j.dump(4);
	return true;
	
}

bool SceneManager::RegisterSceneFromJson(const std::filesystem::path& filePath)
{
	if (filePath.empty())
	{
		return false;
	}

	const auto sceneName = filePath.stem().string();
	if (FindScenePathByName(sceneName))
	{
		return false;
	}

	
	if (!std::filesystem::exists(filePath))
	{
		return false;
	}

	m_Scenes.emplace(sceneName, filePath);

	return true;
}

