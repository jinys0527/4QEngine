#include "pch.h"
#include "GameApplication.h"
#include "GameObject.h"
//#include "Reflection.h"
#include "Engine.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "ServiceRegistry.h"
#include "SoundManager.h"
#include "InputManager.h"
#include "CameraComponent.h"
#include "CameraObject.h"
#include "UIManager.h"


bool GameApplication::Initialize()
{
	const wchar_t* className = L"PDA";
	const wchar_t* windowName = L"PDA";

	if (false == Create(className, windowName, 1920, 1080)) // 해상도 변경
	{
		return false;
	}

	m_AssetLoader = &m_Services.Get<AssetLoader>();
	m_AssetLoader->LoadAll();
	m_SoundManager = &m_Services.Get<SoundManager>();
	m_SoundManager->Init();

	m_Services.Get<SoundManager>().Init();
	m_Renderer.InitializeTest(m_hwnd, m_width, m_height, m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());
	m_SceneManager.Initialize();
	m_InputManager = &m_Services.Get<InputManager>();
	return true;
}

void GameApplication::Run()
{
	MSG msg = { 0 };

	while (WM_QUIT != msg.message && !m_SceneManager.ShouldQuit())
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (false == m_InputManager->OnHandleMessage(msg))
				TranslateMessage(&msg);

			DispatchMessage(&msg);
		}
		else
		{
			m_Engine.UpdateTime();
			Update();
			m_Engine.UpdateInput();
			UpdateLogic();
			Render();
		}
	}
}

void GameApplication::Finalize()
{
	__super::Destroy();
}

bool GameApplication::OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_SYSKEYDOWN && wparam == VK_RETURN && (lparam & (1 << 29)))
	{
		return true;
	}
	return false;
}

void GameApplication::UpdateLogic()
{
	m_SceneManager.ChangeScene();
}

void GameApplication::Update()
{
	ApplySceneBGM();
	float dTime = m_Engine.GetTime();
	dTime *= m_GameSpeed;
	//m_Engine.UpdateInput();
	m_SceneManager.StateUpdate(dTime);
	m_SceneManager.Update(dTime);
	
	m_Services.Get<SoundManager>().Update();
	// FixedUpdate
	{

		while (m_fFrameCount >= 0.016f)
		{
			m_fFrameCount -= 0.016f;
		}

	}
}

void GameApplication::ApplySceneBGM()
{
	auto currentScene = m_SceneManager.GetCurrentScene();
	if (!currentScene)
	{
		return;
	}
}

	m_LastSceneName = sceneName;
	auto it = m_SceneBGMMap.find(sceneName);
	if (it == m_SceneBGMMap.end())
	{
		return;
	}

	m_SoundManager->BGM_Shot(it->second, m_SceneChangeBGMFadeTime);
}

void GameApplication::Render()
{
	//m_Engine.GetRenderer().SetTransform(D2D1::Matrix3x2F::Identity());

	//m_Engine.GetRenderer().RenderBegin();

	m_SceneManager.Render();

	//m_Engine.GetRenderer().RenderEnd(false);

#ifdef _EDITOR
	RenderImGUI();
#endif

	scene->Render(m_FrameData);
	m_FrameData.context.frameIndex = static_cast<UINT32>(m_FrameIndex++);
	m_FrameData.context.deltaTime = m_Engine.GetTimer().DeltaTime();
	m_Renderer.RenderFrame(m_FrameData);
	m_Renderer.RenderToBackBuffer();
	Flip(m_Renderer.GetSwapChain().Get());
}

void GameApplication::OnResize(int width, int height)
{
	__super::OnResize(width, height);
	m_InputManager.SetViewportRect({ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) });
	m_Services.Get<UIManager>().SetViewportSize(UISize{ static_cast<float>(width), static_cast<float>(height) });

	if (m_RendererInitialized && width > 0 && height > 0)
	{
		m_Renderer.ResetRenderTarget(width, height);
	}
}

void GameApplication::OnClose()
{
}
