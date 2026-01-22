
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


bool GameApplication::Initialize()
{
	const wchar_t* className = L"APT";
	const wchar_t* windowName = L"APT";

	if (false == Create(className, windowName, 1920, 1080)) // 해상도 변경
	{
		return false;
	}
	m_Engine.CreateDevice(m_hwnd);

	m_AssetLoader = &m_Services.Get<AssetLoader>();
	m_AssetLoader->LoadAll();
	m_SoundManager = &m_Services.Get<SoundManager>();
	m_SoundManager->Init();

	m_Services.Get<SoundManager>().Init();
	m_Renderer.InitializeTest(m_hwnd, m_width, m_height, m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());
	m_SceneManager.Initialize();
	// GameManager에 SceneManager 등록

	m_SceneRenderTarget.SetDevice(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());

	return true;
}

void GameApplication::Run()
{
	MSG msg = { 0 };

	while (WM_QUIT != msg.message && !m_SceneManager.ShouldQuit())
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (false == m_InputManager.OnHandleMessage(msg)) {
				TranslateMessage(&msg);
			}
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
	return false;
}

void GameApplication::UpdateLogic()
{
	m_SceneManager.ChangeScene();
}

void GameApplication::Update()
{

	float dTime = m_Engine.GetTime();
	dTime *= m_GameSpeed;
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

void GameApplication::Render()
{
	if (!m_Engine.GetD3DDXDC()) return;
	//m_Engine.GetRenderer().RenderBegin();

	ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
	m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
	SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());

	ClearBackBuffer(COLOR(0.12f, 0.12f, 0.12f, 1.0f), m_Engine.GetD3DDXDC(), *rtvs);

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}
	// 현재 Scene의 Camera 받기
	if (auto gameCamera = scene->GetGameCamera())
	{
		if (auto* cameraComponent = gameCamera->GetComponent<CameraComponent>())
		{
			cameraComponent->SetViewport({ static_cast<float>(m_width), static_cast<float>(m_height) });
		}
	}

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
}

void GameApplication::OnClose()
{
}
