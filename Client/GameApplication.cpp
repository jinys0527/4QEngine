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


bool GameApplication::Initialize()
{
	const wchar_t* className = L"APT";
	const wchar_t* windowName = L"APT";

	if (false == Create(className, windowName, 1920, 1080)) // 해상도 변경
	{
		return false;
	}

	//m_Renderer.Initialize(m_hwnd);
	//m_Engine.GetAssetManager().Init(L"../Resource");
	//m_Engine.GetSoundAssetManager().Init(L"../Sound");
	m_Services.Get<SoundManager>().Init();
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
	return false;
}

void GameApplication::UpdateLogic()
{
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
	//m_Engine.GetRenderer().SetTransform(D2D1::Matrix3x2F::Identity());

	//m_Engine.GetRenderer().RenderBegin();

	m_SceneManager.Render();

	//m_Engine.GetRenderer().RenderEnd(false);
	//m_Engine.GetRenderer().Present();
}


void GameApplication::OnResize(int width, int height)
{
	__super::OnResize(width, height);
}

void GameApplication::OnClose()
{
}
