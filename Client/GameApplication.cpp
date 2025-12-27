#include "pch.h"
#include "GameApplication.h"
#include "GameObject.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


bool GameApplication::Initialize()
{
	const wchar_t* className = L"PDA";
	const wchar_t* windowName = L"PDA";

	if (false == Create(className, windowName, 1920, 1080))
	{
		return false;
	}

	//m_Engine.GetRenderer().Initialize(m_hwnd);

	//m_Engine.GetAssetManager().Init(L"../Resource");
	//m_Engine.GetSoundAssetManager().Init(L"../Sound");

	m_SceneManager.Initialize();

#ifdef _EDITOR
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(m_hwnd);

	ImGui_ImplDX11_Init(m_Engine.GetRenderer().GetD3DDevice(), m_Engine.GetRenderer().GetD3DContext());

	ID3D11RenderTargetView* rtvs[] = { m_Engine.GetRenderer().GetD3DRenderTargetView() };
#endif

	return true;
}

void GameApplication::Run()
{
	MSG msg = { 0 };

	while (WM_QUIT != msg.message && !m_SceneManager.ShouldQuit())
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
#ifndef _EDITOR
			if (false == m_Engine.GetInputManager().OnHandleMessage(msg))
#endif
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

#ifdef _EDITOR
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
}

bool GameApplication::OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
#ifdef _EDITOR
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true; // ImGui가 메시지를 처리했으면 true 반환
	}
#endif
	return false;
}

void GameApplication::UpdateLogic()
{
}

void GameApplication::Update()
{
#ifndef _EDITOR
	m_SceneManager.Update(m_Engine.GetTimer().DeltaTime());
	m_SoundManager.Update();
	// FixedUpdate
	{

		while (m_fFrameCount >= 0.016f)
		{
			m_fFrameCount -= 0.016f;
		}

	}
#else
#endif
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

	//m_Engine.GetRenderer().Present();
}

#ifdef _EDITOR
void GameApplication::RenderImGUI()
{
	ID3D11DeviceContext* pd3dDeviceContext = nullptr;
	pd3dDeviceContext = m_Engine.GetRenderer().GetD3DContext();
	ID3D11RenderTargetView* rtvs[] = { m_Engine.GetRenderer().GetD3DRenderTargetView() };

	if (pd3dDeviceContext == nullptr || rtvs[0] == nullptr)
	{
		return; // 렌더링 컨텍스트나 뷰가 없으면 리턴
	}
	m_Engine.GetRenderer().GetD3DContext()->OMSetRenderTargets(1, rtvs, nullptr);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	m_Editor.Update();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

}
#endif

void GameApplication::OnResize(int width, int height)
{
	__super::OnResize(width, height);
}

void GameApplication::OnClose()
{
}
