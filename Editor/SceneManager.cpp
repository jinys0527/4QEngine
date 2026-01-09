//editor
#include "pch.h"
#include "SceneManager.h"
#include "DefaultScene.h"
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
	/*if (m_Scenes.empty()) {

	}*/
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

//std::shared_ptr<Scene> SceneManager::AddScene(const std::string& name, std::shared_ptr<Scene> scene)
//{
//	m_Scenes[name] = scene;
//
//	//m_Scenes[name]->SetGameManager(&m_GameManager);
//
//	return m_Scenes[name];
//}

void SceneManager::SetCurrentScene(std::shared_ptr<Scene> scene)
{
		m_CurrentScene = scene;
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

void SceneManager::LoadSceneFromJson()
{	// 선택했을때 해당 Scene을 Deserialize 해서 
	// 현재 씬으로

}

void SceneManager::SetChangeScene(std::string name)
{
	m_ChangeSceneName = name;
}
