#include "pch.h"
#include "SceneManager.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include "UIManager.h"
#include "DefaultScene.h"

void SceneManager::Initialize()
{
	/*m_UIManager.Start();
	m_UIManager.SetCurrentScene("TitleScene");*/
	m_UIManager = &m_Services.Get<UIManager>();
	m_GameManager = &m_Services.Get<GameManager>();
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
	if (m_UIManager)
	{
		m_UIManager->BuildUIFrameData(frameData);
	}
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

	return m_Scenes[name];
}

void SceneManager::SetCurrentScene(const std::string& name)
{
	auto it = m_Scenes.find(name);
	if (it != m_Scenes.end())
	{
		m_CurrentScene = it->second;
		
		//m_Camera = m_CurrentScene->GetMainCamera();
		//m_Renderer.SetCamera(m_Camera);
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
		//m_Camera = m_CurrentScene->GetMainCamera();
		//m_Renderer.SetCamera(m_Camera);
		m_UIManager->SetCurrentScene(name);
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
