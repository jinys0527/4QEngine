//editor
#include "pch.h"
#include "SceneManager.h"
#include "DefaultScene.h"
#include "json.hpp"
#include <fstream>
//editor 용으로 개발 필요함 - 편집할 Scene 선택, 생성
void SceneManager::Initialize()
{
	m_SoundManager.Init();

	/*m_UIManager.Start();
	m_UIManager.SetCurrentScene("TitleScene");*/

	// 씬없는 경우.. 인데
	// ★★★★★★★★★★★★★★
	// Editor의 경우 m_Scenes 로 여러개를 들고 있을 필요가 없음.
	// 자기가 쓰는 Scene 하나만 있으면됨.
	// 다른 Scene의 경우 경로에서 Scenedata json으로 띄우기만 하면 됨.
	// 그냥 Default 생성
	auto emptyScene = std::make_shared<DefaultScene>(m_EventDispatcher, m_SoundManager, m_UIManager);
	emptyScene->SetName("Untitled Scene");
	emptyScene->Initialize();

	// editor는 Current Scene만 띄운다.
	SetCurrentScene(emptyScene);
}

void SceneManager::Update(float deltaTime)
{
	if (!m_CurrentScene)
		return;

	static float totalTime = 0;
	totalTime += deltaTime;
	if (totalTime >= 0.016f)
		m_CurrentScene->FixedUpdate();
	m_CurrentScene->Update(deltaTime);
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
	m_CurrentScene = scene;
	m_Camera = m_CurrentScene->GetGameCamera();
	//m_Renderer.SetCamera(m_Camera);

}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() const
{
	return m_CurrentScene;
}

void SceneManager::ChangeScene(const std::string& name)
{

}

void SceneManager::ChangeScene()
{
	ChangeScene(m_ChangeSceneName);
}

void SceneManager::SetChangeScene(std::string name)
{
	m_ChangeSceneName = name;
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

	auto loadedScene = std::make_shared<DefaultScene>(m_EventDispatcher, m_SoundManager, m_UIManager);
	loadedScene->SetName(filePath.stem().string());
	loadedScene->Initialize();
	loadedScene->Deserialize(j);
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


