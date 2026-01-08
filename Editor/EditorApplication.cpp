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
	m_Renderer.InitializeTest(m_hwnd,m_width,m_height);  // Device 생성
	m_SceneManager.Initialize();

	ImGui::CreateContext();
	ImGuiIO& m_io = ImGui::GetIO();
	m_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	m_io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark(); 
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(g_pDevice.Get(), g_pDXDC.Get()); //★ 일단 임시 Renderer의 Device사용, 엔진에서 받는 걸로 수정해야됨
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
	m_SceneManager.Render(); // Scene 전체 그리고
	RenderImGUI();
	Flip(); //★
}

void EditorApplication::RenderImGUI() {
	//★★
	
	if (!g_pDXDC) return; //★
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	CreateDockSpace();
	static bool firstTime = true;
	if (firstTime)
	{
		SetupEditorDockLayout(); // DockBuilder
		firstTime = false;
	}


	DrawHierarchy();
	DrawInspector();

	//흐름만 참조. 추후 우리 형태에 맞게 개발 필요
	const bool viewportChanged = m_Viewport.Draw(m_SceneRenderTarget);
	if (viewportChanged)
	{
		UpdateSceneViewport();
	}
	
	RenderSceneView(); //Scene그리기
	
	ImGui::Render();  // Gui들그리기
	
	ID3D11RenderTargetView* rtvs[] = { g_pRTView.Get() };
	g_pDXDC->OMSetRenderTargets(1, rtvs, nullptr);
	SetViewPort(m_width, m_height);

	
	ImGuiIO& m_io = ImGui::GetIO();

	if (m_io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		ClearBackBuffer(COLOR(0.1f, 0.1f, 0.12f, 1.0f));
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

}

// 게임화면 
void EditorApplication::RenderSceneView() {

	/*if (!m_SceneRenderTarget.IsValid())
	{
		return;
	}

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}

	scene->BuildFrameData(m_FrameData);
	m_FrameData.context.frameIndex = static_cast<UINT32>(m_FrameIndex++);
	m_FrameData.context.deltaTime = m_Engine.GetTimer().DeltaTime();

	m_Renderer.InitVB(m_FrameData);
	m_Renderer.InitIB(m_FrameData);

	m_SceneRenderTarget.Bind();
	m_SceneRenderTarget.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));

	m_Renderer.RenderFrame(m_FrameData);*/
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

		// GameObjects map 가져와
	for (const auto& [name, Object] : scene->GetGameObjects()) {
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
	const auto& gameObjects = scene->GetGameObjects();
	const auto it = gameObjects.find(m_SelectedObjectName);

	// 선택된 오브젝트가 없거나, 실체가 없는 경우
	if (it == gameObjects.end() || !it->second) //second == Object 포인터
	{	
		ImGui::Text("No Selected GameObject");
		ImGui::End();
		return;
	}

	ImGui::Text("Name : %s", it->second->GetName().c_str());
	ImGui::Separator();
	ImGui::Text("Components");

	for (const auto& typeName : it->second->GetComponentTypeNames()) {
		ImGui::BulletText("%s", typeName.c_str());

		// 여기서 각 Component별 Property와 조작까지 생성?
	}



}

void EditorApplication::CreateDockSpace()
{
	static bool dockspaceOpen = true;

	ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_MenuBar |
		ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	ImGui::Begin("DockSpaceRoot", &dockspaceOpen, windowFlags);
	ImGui::PopStyleVar(2);

	ImGuiID dockspaceID = ImGui::GetID("EditorDockSpace");
	ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f));

	ImGui::End();
}

void EditorApplication::SetupEditorDockLayout()
{	// 초기 창 셋팅
	ImGuiID dockspaceID = ImGui::GetID("EditorDockSpace");

	ImGui::DockBuilderRemoveNode(dockspaceID);
	ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->WorkSize);

	ImGuiID dockMain = dockspaceID;
	ImGuiID dockLeft, dockRight, dockBottom;

	// 분할
	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.2f, &dockLeft, &dockMain);
	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.25f, &dockRight, &dockMain);
	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.3f, &dockBottom, &dockMain);

	// 창 배치
	ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
	ImGui::DockBuilderDockWindow("Inspector", dockRight);
	ImGui::DockBuilderDockWindow("Viewport", dockMain);

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
