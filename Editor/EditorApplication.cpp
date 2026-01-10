#include "pch.h"
#include "EditorApplication.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "Renderer.h"
#include "Scene.h"
#include "DX11.h"

#include "pch.h"
#include "EditorApplication.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "Renderer.h"
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
	///m_hwnd
	//m_Engine.GetAssetManager().Init(L"../Resource");
	//m_Engine.GetSoundAssetManager().Init(L"../Sound");
	m_Engine.CreateDevice(m_hwnd);							//엔진 Device, DXDC생성
	m_Renderer.InitializeTest(m_hwnd, m_width, m_height, m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());  // Device 생성
	m_SceneManager.Initialize();

	m_SceneRenderTarget.SetDevice(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;				// ini 사용 안함
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC()); //★ 일단 임시 Renderer의 Device사용, 엔진에서 받는 걸로 수정해야됨
	//ImGui_ImplDX11_Init(m_Engine.Get3DDevice(),m_Engine.GetD3DDXDC());
	//RT 받기

	//초기 세팅 값으로 창 배치


	return true;
}


bool EditorApplication::OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true; // ImGui가 메시지를 처리했으면 true 반환
	}
}



void EditorApplication::Run() {
	//실행 루프
	MSG msg = { 0 };
	while (WM_QUIT != msg.message /*&& !m_SceneManager.ShouldQuit()*/) {

		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
			// Window Message 해석
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			m_Engine.UpdateTime();
			Update();
			m_Engine.UpdateInput();
			//UpdateLogic();  //★
			Render();

		}
	}
}



void EditorApplication::Finalize() {
	__super::Destroy();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//그외 메모리 해제
}

void EditorApplication::UpdateInput()
{

}

void EditorApplication::Update()
{
	m_SceneManager.Update(m_Engine.GetTimer().DeltaTime());
	m_SoundManager.Update();
}

void EditorApplication::Render() {
	if (!m_Engine.GetD3DDXDC()) return; //★


	ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get()};
	m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
	SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());

	ClearBackBuffer(COLOR(0.1f, 0.1f, 0.12f, 1.0f), m_Engine.GetD3DDXDC(), *rtvs);

	m_SceneManager.Render(); // Scene 전체 그리고

	RenderImGUI();

	Flip(m_Renderer.GetSwapChain().Get()); //★
}

void EditorApplication::RenderImGUI() {
	//★★
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	CreateDockSpace();

	DrawHierarchy();
	DrawInspector();
	DrawFolderView();

	RenderSceneView(); //Scene그리기

	//흐름만 참조. 추후 우리 형태에 맞게 개발 필요
	const bool viewportChanged = m_Viewport.Draw(m_SceneRenderTarget);
	if (viewportChanged)
	{
		UpdateSceneViewport();
	}
	
	
	// DockBuilder
	static bool dockBuilt = true;
	if (dockBuilt)
	{
		//std::cout << "Layout Init" << std::endl;
		SetupEditorDockLayout();
		dockBuilt = false;
	}


	ImGui::Render();  // Gui들그리기

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();

		// main back buffer 상태 복구 // 함수로 묶기
		ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
		m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
		m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
		SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());
	}

}

// 게임화면 
void EditorApplication::RenderSceneView() {

	//if (!m_SceneRenderTarget.IsValid())
	//{
	//	return;
	//}

	//auto scene = m_SceneManager.GetCurrentScene();
	//if (!scene)
	//{
	//	return;
	//}

	//scene->BuildFrameData(m_FrameData);
	m_FrameData.context.frameIndex = static_cast<UINT32>(m_FrameIndex++);
	m_FrameData.context.deltaTime = m_Engine.GetTimer().DeltaTime();

	//m_Renderer.InitVB(m_FrameData);
	//m_Renderer.InitIB(m_FrameData);


	m_SceneRenderTarget.Bind();
	m_SceneRenderTarget.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));

	m_Renderer.RenderFrame(m_FrameData, m_SceneRenderTarget);


	// 복구
	ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
	m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
	SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());
}


void EditorApplication::DrawHierarchy() {


	ImGui::Begin("Hierarchy");

	auto scene = m_SceneManager.GetCurrentScene();

	// Scene이 없는 경우
	if (!scene) {
		ImGui::Text("No Current Scene");
		ImGui::End();
		return;
	}

	// GameObjects map 가져와	Opaque
	for (const auto& [name, Object] : scene->GetOpaqueObjects()) {
		const bool selected = (m_SelectedObjectName == name);
		if (ImGui::Selectable(name.c_str(), selected)) {
			m_SelectedObjectName = name;
		}
	}

	//Transparent
	for (const auto& [name, Object] : scene->GetTransparentObjects()) {
		const bool selected = (m_SelectedObjectName == name);
		if (ImGui::Selectable(name.c_str(), selected)) {
			m_SelectedObjectName = name;
		}
	}

	ImGui::End();

}

void EditorApplication::DrawInspector() {
	ImGui::Begin("Inspector");
	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene) {
		ImGui::Text("No Selected Object");
		ImGui::End();
		return;
	}

	// hierarchy 에서 선택한 object 
// Opaque
	const auto& opaqueObjects = scene->GetOpaqueObjects();
	const auto opaqueIt = opaqueObjects.find(m_SelectedObjectName);

	// Transparent
	const auto& transparentObjects = scene->GetTransparentObjects();
	const auto transparentIt = transparentObjects.find(m_SelectedObjectName);

	// 선택된 오브젝트가 없거나, 실체가 없는 경우
	if ((opaqueIt == opaqueObjects.end() || !opaqueIt->second) && (transparentIt == transparentObjects.end() || !transparentIt->second)) //second == Object 포인터
	{
		ImGui::Text("No Selected GameObject");
		ImGui::End();
		return;
	}

	auto it = (opaqueIt != opaqueObjects.end() && opaqueIt->second) ? opaqueIt : transparentIt;

	ImGui::Text("Name : %s", it->second->GetName().c_str());
	ImGui::Separator();
	ImGui::Text("Components");

	for (const auto& typeName : it->second->GetComponentTypeNames()) {
		ImGui::BulletText("%s", typeName.c_str());

		// 여기서 각 Component별 Property와 조작까지 생성?
	}
	ImGui::End();
}

void EditorApplication::DrawFolderView()
{
	ImGui::Begin("Folder");

	ImGui::Text("Need Logic");

	ImGui::End();
}


void EditorApplication::CreateDockSpace()
{
	ImGui::DockSpaceOverViewport(
		ImGui::GetID("EditorDockSpace"),
		ImGui::GetMainViewport(),
		ImGuiDockNodeFlags_PassthruCentralNode
	);
}

void EditorApplication::SetupEditorDockLayout()
{	// 초기 창 셋팅
	ImGuiID dockspaceID = ImGui::GetID("EditorDockSpace");

	ImGui::DockBuilderRemoveNode(dockspaceID);
	ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->WorkSize);

	ImGuiID dockMain = dockspaceID; // dockMain을 쪼개서 쓰는 것.
	ImGuiID dockLeft,
			dockRight,
			dockRightA,
			dockRightB,
			dockBottom;



	// 분할   ( 어떤 영역을, 어느방향에서, 비율만큼, 뗀 영역의 이름, Dockspacemain)
	// 나눠지는 순서도 중요
	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.2f, &dockLeft, &dockMain);

	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.20f, &dockRight, &dockMain); //Right 20%
	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.2f, &dockBottom, &dockMain);  //Down 20%

	ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Left, 0.50f, &dockRightA, &dockMain); // Right 10%
	ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Right, 0.50f, &dockRightB, &dockMain);// Right 10%
	
	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.10f, &dockRight, &dockMain);
	
	 //Down 30%

	// 창 할당
	ImGui::DockBuilderDockWindow("Hierarchy", dockRightA);
	ImGui::DockBuilderDockWindow("Inspector", dockRightB);
	ImGui::DockBuilderDockWindow("Folder", dockBottom);
	ImGui::DockBuilderDockWindow("Game", dockMain);

	ImGui::DockBuilderFinish(dockspaceID);
}

void EditorApplication::UpdateSceneViewport()
{
	// m_Veiwport = editor Viewport
	const ImVec2 size = m_Viewport.GetViewportSize();
	const UINT width = static_cast<UINT>(size.x);
	const UINT height = static_cast<UINT>(size.y);
	if (width == 0 || height == 0)
	{
		return;
	}

	/*m_SceneRenderTarget.Resize(width, height);

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}

	auto* camera = scene->GetMainCamera();
	if (!camera)
	{
		return;
	}

	if (auto* cameraComponent = camera->GetComponent<CameraComponent>())
	{
		cameraComponent->SetViewportSize(static_cast<float>(width), static_cast<float>(height));
	}*/
}



void EditorApplication::OnResize(int width, int height)
{
	__super::OnResize(width, height);
}

void EditorApplication::OnClose()
{
	m_SceneManager.RequestQuit();
}
