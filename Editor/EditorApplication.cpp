#include "pch.h"
#include "EditorApplication.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "Scene.h"
#include "DX11.h"


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



bool EditorApplication::Initialize()
{
	const wchar_t* className = L"MIEditor";
	const wchar_t* windowName = L"MIEditor";

	if (false == Create(className, windowName, 1920, 1080))
	{
		return false;
	}

	//m_Engine.GetRenderer().Initialize(m_hwnd);

	//m_Engine.GetAssetManager().Init(L"../Resource");
	//m_Engine.GetSoundAssetManager().Init(L"../Sound");

	m_SceneManager.Initialize();

	ImGui::CreateContext();
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(g_pDevice.Get(), g_pDXDC.Get()); //★ 일단 임시 Renderer의 Device사용, 엔진에서 받는 걸로 수정해야됨
	//ID3D11RenderTargetView* rtvs[] = { m_Engine.GetRenderer().GetD3DRenderTargetView() };


	return true;
}


bool EditorApplication::OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true; // ImGui가 메시지를 처리했으면 true 반환
	}
}

