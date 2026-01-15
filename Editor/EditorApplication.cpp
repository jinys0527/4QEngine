#include "pch.h"
#include "EditorApplication.h"
#include "CameraObject.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "MaterialComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMeshRenderer.h"
#include "AnimationComponent.h"
#include "CameraComponent.h"
#include "GameObject.h"
#include "Reflection.h"
#include "ServiceRegistry.h"
#include "AssetLoader.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "Scene.h"
#include "DX11.h"
#include "Importer.h"
#include "Util.h"
#include "json.hpp"
#include "ImGuizmo.h"
#include "MathHelper.h"



#define DRAG_SPEED 0.01f
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// ImGUI 창그리기
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

		ImportAll();
		m_AssetLoader = &m_Services.Get<AssetLoader>();
		m_AssetLoader->LoadAll();
		m_SoundManager = &m_Services.Get<SoundManager>();
		m_SoundManager->Init();

		m_Renderer.InitializeTest(m_hwnd, m_width, m_height, m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());  // Device 생성
		m_SceneManager.Initialize();

		

		m_SceneRenderTarget.SetDevice(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());
		m_SceneRenderTarget_edit.SetDevice(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());

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

	bool EditorApplication::OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
		{
			return true; // ImGui가 메시지를 처리했으면 true 반환
		}

		return false;
	}



	void EditorApplication::UpdateInput()
	{

	}

	void EditorApplication::Update()
	{
		m_SceneManager.Update(m_Engine.GetTimer().DeltaTime());
		m_SoundManager->Update();
	}

void EditorApplication::UpdateSceneViewport()
{
	// m_Veiwport = editor Viewport
	const ImVec2 editorSize = m_EditorViewport.GetViewportSize();
	const ImVec2 gameSize = m_GameViewport.GetViewportSize();
	const UINT editorWidth = static_cast<UINT>(editorSize.x);
	const UINT editorHeight = static_cast<UINT>(editorSize.y);
	const UINT gameWidth = static_cast<UINT>(gameSize.x);
	const UINT gameHeight = static_cast<UINT>(gameSize.y);

		auto scene = m_SceneManager.GetCurrentScene();
		if (!scene)
		{
			return;
		}

	if (editorWidth != 0 && editorHeight != 0)
	{
		//return;
		if (auto editorCamera = scene->GetEditorCamera())
		{
			if (auto* cameraComponent = editorCamera->GetComponent<CameraComponent>())
			{
				cameraComponent->SetViewport({ static_cast<float>(editorWidth), static_cast<float>(editorHeight) });
			}
		}
	}

	if (gameWidth != 0 && gameHeight != 0)
	{
		if (auto gameCamera = scene->GetGameCamera())
		{
			if (auto* cameraComponent = gameCamera->GetComponent<CameraComponent>())
			{
				cameraComponent->SetViewport({ static_cast<float>(gameWidth), static_cast<float>(gameHeight) });
			}
		}
	}

}

void EditorApplication::UpdateEditorCamera()
{
	if (!m_EditorViewport.IsHovered())
	{
		return;
	}

		auto scene = m_SceneManager.GetCurrentScene();
		if (!scene)
		{
			return;
		}

		auto camera = scene->GetEditorCamera();
		if (!camera)
		{
			return;
		}

		auto* cameraComponent = camera->GetComponent<CameraComponent>();
		if (!cameraComponent)
		{
			return;
		}

		ImGuiIO& io = ImGui::GetIO();
		const float deltaTime = (io.DeltaTime > 0.0f) ? io.DeltaTime : m_Engine.GetTimer().DeltaTime();

		XMFLOAT3 eye = cameraComponent->GetEye();
		XMFLOAT3 look = cameraComponent->GetLook();
		XMFLOAT3 up = cameraComponent->GetUp();

		XMVECTOR eyeVec = XMLoadFloat3(&eye);
		XMVECTOR lookVec = XMLoadFloat3(&look);
		XMVECTOR upVec = XMVector3Normalize(XMLoadFloat3(&up));
		XMVECTOR forwardVec = XMVector3Normalize(XMVectorSubtract(lookVec, eyeVec));
		XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(upVec, forwardVec));

		bool updated = false;

	if (io.MouseDown[1])
	{
		const float rotationSpeed = 0.003f;
		const float yaw = io.MouseDelta.x * rotationSpeed;
		const float pitch = io.MouseDelta.y * rotationSpeed;

			if (yaw != 0.0f || pitch != 0.0f)
			{
				XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				const XMMATRIX yawRotation = XMMatrixRotationAxis(upVec, yaw);
				const XMMATRIX pitchRotation = XMMatrixRotationAxis(rightVec, pitch);
				XMMATRIX transform = pitchRotation * yawRotation;
				forwardVec = XMVector3Normalize(XMVector3TransformNormal(forwardVec, transform));
				rightVec = XMVector3Normalize(XMVector3Cross(worldUp, forwardVec));
				upVec = XMVector3Normalize(XMVector3Cross(forwardVec, rightVec));

				lookVec = XMVectorAdd(eyeVec, forwardVec);
				updated = true;

			}

			const float baseSpeed = 6.0f;
			const float speedMultiplier = io.KeyShift ? 3.0f : 1.0f;
			const float moveSpeed = baseSpeed * speedMultiplier;

		XMVECTOR moveVec = XMVectorZero();
		if (ImGui::IsKeyDown(ImGuiKey_W))
		{
			moveVec = XMVectorAdd(moveVec, forwardVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_S))
		{
			moveVec = XMVectorSubtract(moveVec, forwardVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_D))
		{
			moveVec = XMVectorAdd(moveVec, rightVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_A))
		{
			moveVec = XMVectorSubtract(moveVec, rightVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_E))
		{
			moveVec = XMVectorAdd(moveVec, upVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_Q))
		{
			moveVec = XMVectorSubtract(moveVec, upVec);
		}
		if (XMVectorGetX(XMVector3LengthSq(moveVec)) > 0.0f)
		{
			moveVec = XMVector3Normalize(moveVec);
			const XMVECTOR scaledMove = XMVectorScale(moveVec, moveSpeed * deltaTime);
			eyeVec = XMVectorAdd(eyeVec, scaledMove);
			lookVec = XMVectorAdd(lookVec, scaledMove);
			updated = true;
		}
	}

		if (io.MouseDown[2])
		{
			const float panSpeed = 0.01f;
			const XMVECTOR panRight = XMVectorScale(rightVec, -io.MouseDelta.x * panSpeed);
			const XMVECTOR panUp = XMVectorScale(upVec, io.MouseDelta.y * panSpeed);
			const XMVECTOR pan = XMVectorAdd(panRight, panUp);
			eyeVec = XMVectorAdd(eyeVec, pan);
			lookVec = XMVectorAdd(lookVec, pan);
			updated = true;
		}

		if (io.MouseWheel != 0.0f)
		{
			const float zoomSpeed = 4.0f;
			const XMVECTOR dolly = XMVectorScale(forwardVec, io.MouseWheel * zoomSpeed);
			eyeVec = XMVectorAdd(eyeVec, dolly);
			lookVec = XMVectorAdd(lookVec, dolly);
			updated = true;
		}

		if (updated)
		{
			upVec = XMVector3Normalize(XMVector3Cross(forwardVec, rightVec));
			XMStoreFloat3(&eye, eyeVec);
			XMStoreFloat3(&look, lookVec);
			XMStoreFloat3(&up, upVec);
			cameraComponent->SetEyeLookUp(eye, look, up);
		}
	}

	void EditorApplication::Render() {
		if (!m_Engine.GetD3DDXDC()) return; //★


		ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get()};
		m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
		SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());

	ClearBackBuffer(COLOR(0.12f, 0.12f, 0.12f, 1.0f), m_Engine.GetD3DDXDC(), *rtvs);

	//m_SceneManager.Render(); // Scene 전체 그리고

		RenderImGUI();

		Flip(m_Renderer.GetSwapChain().Get()); //★
	}

	void EditorApplication::RenderImGUI() {
		//★★
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

	CreateDockSpace();
	DrawMainMenuBar();
	DrawHierarchy();
	
	DrawInspector();
	RenderSceneView();
	DrawFolderView();
	DrawResourceBrowser();
	
	 //Scene그리기

	//흐름만 참조. 추후 우리 형태에 맞게 개발 필요
	const bool gameViewportChanged = m_GameViewport.Draw(m_SceneRenderTarget);
	const bool editorViewportChanged = m_EditorViewport.Draw(m_SceneRenderTarget_edit);
	
	if (editorViewportChanged || gameViewportChanged)
	{
		UpdateSceneViewport();
	}
	
		UpdateEditorCamera();
		DrawGizmo();
		

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
		//m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
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

	m_SceneRenderTarget.Bind();
	m_SceneRenderTarget.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));

		m_SceneRenderTarget_edit.Bind();
		m_SceneRenderTarget_edit.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));


	auto scene = m_SceneManager.GetCurrentScene();
	if (scene)
	{
		scene->Render(m_FrameData);
	}

		m_Renderer.RenderFrame(m_FrameData, m_SceneRenderTarget, m_SceneRenderTarget_edit);

	

		if (!scene)
		{
			return;
		}

		auto editorCamera = scene->GetEditorCamera().get();
		if (!editorCamera)
		{
			return;
		}

	if (auto* cameraComponent = editorCamera->GetComponent<CameraComponent>())
	{
		const ImVec2 editorSize = m_EditorViewport.GetViewportSize();
		if (editorSize.x > 0.0f && editorSize.y > 0.0f)
		{
			cameraComponent->SetViewport({ editorSize.x, editorSize.y });
		}
	}
	//scene->Render(m_FrameData);

	m_SceneRenderTarget.Bind();
	m_SceneRenderTarget.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));

	m_SceneRenderTarget_edit.Bind();
	m_SceneRenderTarget_edit.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));

	m_Renderer.RenderFrame(m_FrameData, m_SceneRenderTarget, m_SceneRenderTarget_edit);
	scene->Render(m_FrameData);
	// 복구
	ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
	m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
	SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());
}


	void EditorApplication::DrawMainMenuBar()
	{
		if (ImGui::BeginMainMenuBar())
		{
			auto scene = m_SceneManager.GetCurrentScene();
			const char* sceneName = scene ? scene->GetName().c_str() : "None";
			ImGui::Text("Scene: %s", sceneName);
			ImGui::EndMainMenuBar();
		}
	}

	void EditorApplication::DrawHierarchy() {


		ImGui::Begin("Hierarchy");

		auto scene = m_SceneManager.GetCurrentScene();

		// Scene이 없는 경우는 이젠 없음
		if (!scene) {
			ImGui::Text("Scene Loading Fail");
			ImGui::End();
			return;
		}

		if (scene->GetName() != m_LastSceneName)
		{
			CopyStringToBuffer(scene->GetName(), m_SceneNameBuffer);
			m_LastSceneName = scene->GetName();
		}
		// Scene 이름 변경
		ImGui::Text("Scene");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##SceneName", m_SceneNameBuffer.data(), m_SceneNameBuffer.size());

		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			std::string oldName = scene->GetName();
			std::string newName = m_SceneNameBuffer.data();
			if (!newName.empty() && newName != scene->GetName())
			{
				scene->SetName(newName);
				m_LastSceneName = newName;
				if (!m_CurrentScenePath.empty() && m_CurrentScenePath.stem() == oldName)
				{
					std::filesystem::path renamedPath = m_CurrentScenePath.parent_path() / (newName + m_CurrentScenePath.extension().string());
					if (m_SelectedResourcePath == m_CurrentScenePath)
					{
						m_SelectedResourcePath = renamedPath;
					}
					m_CurrentScenePath = renamedPath;
				}
			}
			else
			{
				CopyStringToBuffer(scene->GetName(), m_SceneNameBuffer);
			}
		}

		// hier창 우클릭 생성, 오브젝트에서 우클릭 Delete 필요( 추후 수정) 
		if (ImGui::Button("Add GameObject")) // Button
		{
			const std::string name = MakeUniqueObjectName(*scene, "GameObject");
			scene->CreateGameObject(name, true); //일단 Opaque // GameObject 생성 후 바꾸는 게 좋아 보임;;  
			//scene->CreateGameObject(name, false); //transparent
			m_SelectedObjectName = name;
		}

		ImGui::Separator();
		std::vector<std::string> pendingDeletes;

		// GameObjects map 가져와	Opaque
		for (const auto& [name, Object] : scene->GetOpaqueObjects()) {
			ImGui::PushID(name.c_str());
			const bool selected = (m_SelectedObjectName == name);
			if (ImGui::Selectable(name.c_str(), selected)) {
				m_SelectedObjectName = name;
			}
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				m_SelectedObjectName = name;
				FocusEditorCameraOnObject(Object);
			}
			if (ImGui::BeginPopupContextItem("ObjectContext"))
			{
				if (ImGui::MenuItem("Delete"))
				{
					pendingDeletes.push_back(name);
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}

		//Transparent
		for (const auto& [name, Object] : scene->GetTransparentObjects()) {
			ImGui::PushID(name.c_str());
			const bool selected = (m_SelectedObjectName == name);
			if (ImGui::Selectable(name.c_str(), selected)) {
				m_SelectedObjectName = name;
			}
			if (ImGui::BeginPopupContextItem("ObjectContext"))
			{
				if (ImGui::MenuItem("Delete"))
				{
					pendingDeletes.push_back(name);
				}
				ImGui::EndPopup();
			}
			ImGui::PopID();
		}

		for (const auto& name : pendingDeletes)
		{
			scene->RemoveGameObjectByName(name);
			if (m_SelectedObjectName == name)
			{
				m_SelectedObjectName.clear();
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
		//second == Object 포인터
		if ((opaqueIt == opaqueObjects.end() || !opaqueIt->second) && (transparentIt == transparentObjects.end() || !transparentIt->second)) 
		{
			ImGui::Text("No Selected GameObject");
			ImGui::End();
			return;
		}

		auto it = (opaqueIt != opaqueObjects.end() && opaqueIt->second) ? opaqueIt : transparentIt;
		auto selectedObject = it->second;

		if (m_LastSelectedObjectName != m_SelectedObjectName)
		{
			CopyStringToBuffer(selectedObject->GetName(), m_ObjectNameBuffer);
			m_LastSelectedObjectName = m_SelectedObjectName;
		}

		ImGui::Text("Name");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("##ObjectName", m_ObjectNameBuffer.data(), m_ObjectNameBuffer.size());
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			std::string newName = m_ObjectNameBuffer.data();
			if (!newName.empty() && newName != selectedObject->GetName())
			{
				const std::string currentName = it->second->GetName();
				if (scene->RenameGameObject(currentName, newName))
				{
					m_SelectedObjectName = newName;
					m_LastSelectedObjectName = newName;
				}
				else
				{
					CopyStringToBuffer(selectedObject->GetName(), m_ObjectNameBuffer);
				}
			}
			else
			{
				CopyStringToBuffer(selectedObject->GetName(), m_ObjectNameBuffer);
			}
		}
		ImGui::Separator();
		ImGui::Text("Components");
		ImGui::Separator();


	for (const auto& typeName : selectedObject->GetComponentTypeNames()) {
		// 여기서 각 Component별 Property와 조작까지 생성?
		ImGui::PushID(typeName.c_str());
		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		const bool nodeOpen = ImGui::TreeNodeEx(typeName.c_str(), flags);
		if (ImGui::BeginPopupContextItem("ComponentContext"))
		{
			//const bool canRemove = (typeName != "TransformComponent"); //일단 Trasnsform은 삭제 막아둠
			/*if (!canRemove)
			{
				ImGui::BeginDisabled();
			}*/

			//삭제
			if (ImGui::MenuItem("Remove Component"))
			{
				selectedObject->RemoveComponentByTypeName(typeName);
			}
		/*	if (!canRemove)
			{
				ImGui::EndDisabled(); 
			}*/
			ImGui::EndPopup();
		}
		if (nodeOpen)
		{
			Component* component = selectedObject->GetComponentByTypeName(typeName);
			auto* typeInfo = ComponentRegistry::Instance().Find(typeName);
			
				if (component && typeInfo)
				{
					auto props = ComponentRegistry::Instance().CollectProperties(typeInfo);

					for (const auto& prop : props)
					{	
						DrawComponentPropertyEditor(component, *prop, *m_AssetLoader);
					}

  					if (auto* meshComponent = dynamic_cast<MeshComponent*>(component))
  					{
  						DrawSubMeshOverridesEditor(*meshComponent, *m_AssetLoader);
  					}
					ImGui::Separator();
				}
				else
				{
					ImGui::TextDisabled("Component data unavailable.");
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		}

	

		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("AddComponentPopup");
		}

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		// Logic 
		const auto existingTypes = selectedObject->GetComponentTypeNames(); //
		const auto typeNames = ComponentRegistry::Instance().GetTypeNames();
		for (const auto& typeName : typeNames)
		{
			const bool hasType = std::find(existingTypes.begin(), existingTypes.end(), typeName) != existingTypes.end();
			//const bool disallowDuplicate = (typeName == "TransformComponent");
			const bool disabled = hasType /*&& disallowDuplicate*/;

				if (disabled)
				{
					ImGui::BeginDisabled();
				}

				if (ImGui::MenuItem(typeName.c_str()))
				{
					auto comp = ComponentFactory::Instance().Create(typeName);
					if (comp)
					{
						selectedObject->AddComponent(std::move(comp));
					}
				}

				if (disabled)
				{
					ImGui::EndDisabled();
				}
			}
			ImGui::EndPopup();
		} // 


		ImGui::End();
	}
	void EditorApplication::DrawFolderView()
	{
		ImGui::Begin("Folder");

		//ImGui::Text("Need Logic");
		//logic
		if (!std::filesystem::exists(m_ResourceRoot)) {
			// resource folder 인식 문제방지
			ImGui::Text("Resources folder not found: %s", m_ResourceRoot.string().c_str());
			ImGui::End();
			return;
		}

		auto createNewScene = [&]()
			{
				const std::string baseName = "NewScene";
				std::filesystem::path newPath;
				for (int index = 0; index < 10000; ++index)
				{
					std::string name = (index == 0) ? baseName : baseName + std::to_string(index);
					newPath = m_ResourceRoot / (name + ".json");
					if (!std::filesystem::exists(newPath))
					{
						break;
					}
				}

				nlohmann::json j;
				j["gameObjects"] = nlohmann::json::object();
				j["gameObjects"]["opaque"] = nlohmann::json::array();
				j["gameObjects"]["transparent"] = nlohmann::json::array();

				std::ofstream out(newPath);
				if (out)
				{
					out << j.dump(4);
					out.close();
					if (m_SceneManager.LoadSceneFromJson(newPath))
					{
						m_CurrentScenePath = newPath;
						m_SelectedResourcePath = newPath;
						m_SelectedObjectName.clear();
						m_LastSelectedObjectName.clear();
						m_ObjectNameBuffer.fill('\0');
					}
				}
			};

		if (ImGui::Button("New Scene"))
		{
			createNewScene();
		}

		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			auto scene = m_SceneManager.GetCurrentScene();
			if (scene)
			{
				std::filesystem::path savePath = m_CurrentScenePath;

				if (savePath.empty())
				{
					savePath = m_ResourceRoot / (scene->GetName() + ".json");
				}
				m_PendingSavePath = savePath;
				m_OpenSaveConfirm = true;
				m_PendingSavePath = savePath;
				m_OpenSaveConfirm = true;
			}
		}

		ImGui::SameLine();
		const bool canDelete = !m_SelectedResourcePath.empty() && m_SelectedResourcePath.extension() == ".json";
		if (!canDelete)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Delete"))
		{
			m_PendingDeletePath = m_SelectedResourcePath;
			m_OpenDeleteConfirm = true;
		}
		if (!canDelete)
		{
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		if (m_CurrentScenePath.empty())
		{
			ImGui::Text("Current Scene: None");
		}
		else
		{
			ImGui::Text("Current Scene: %s", m_CurrentScenePath.filename().string().c_str());
		}

		if (m_OpenSaveConfirm)
		{
			ImGui::OpenPopup("Confirm Save");
			m_OpenSaveConfirm = false;
		}

		if (m_OpenDeleteConfirm)
		{
			ImGui::OpenPopup("Confirm Delete");
			m_OpenDeleteConfirm = false;
		}

		if (ImGui::BeginPopupContextWindow("FolderContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("New Scene"))
			{
				createNewScene();
			}
			ImGui::EndPopup();
		}

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Confirm Save", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Save scene \"%s\"?", m_PendingSavePath.filename().string().c_str());
			if (ImGui::Button("Save"))
			{
				if (m_SceneManager.SaveSceneToJson(m_PendingSavePath))
				{
					m_CurrentScenePath = m_PendingSavePath;
					m_SelectedResourcePath = m_PendingSavePath;
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
		
			if (ImGui::Button("Delete"))
			{
				std::error_code error;
				std::filesystem::remove(m_PendingDeletePath, error);
				if (!error)
				{
					if (m_CurrentScenePath == m_PendingDeletePath)
					{
						m_CurrentScenePath.clear();
					}
					if (m_SelectedResourcePath == m_PendingDeletePath)
					{
						m_SelectedResourcePath.clear();
					}
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::Separator();

		const auto drawDirectory = [&](const auto& self, const std::filesystem::path& dir) -> void
			{
				std::vector<std::filesystem::directory_entry> entries;
				for (const auto& entry : std::filesystem::directory_iterator(dir))
				{
					entries.push_back(entry);
				}

				std::sort(entries.begin(), entries.end(),
					[](const auto& a, const auto& b)
					{
						if (a.is_directory() != b.is_directory())
						{
							return a.is_directory() > b.is_directory();
						}
						return a.path().filename().string() < b.path().filename().string();
					});

				for (const auto& entry : entries)
				{
					const auto name = entry.path().filename().string();
					if (entry.is_directory())
					{
						const std::string label = name;
						const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
						if (ImGui::TreeNodeEx(label.c_str(), flags))
						{
							self(self, entry.path());
							ImGui::TreePop();
						}
					}
					else
					{
						const std::string label = name;
						const bool selected = (m_SelectedResourcePath == entry.path());
						const bool isSceneFile = entry.path().extension() == ".json";
						ImGui::PushID(label.c_str());
						if (isSceneFile && selected)
						{
							ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
							ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
						}

						bool clicked = false;
						if (isSceneFile)
						{
							clicked = ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
						}
						else
						{
							clicked = ImGui::Selectable(label.c_str(), selected);
						}

						if (isSceneFile && selected)
						{
							ImGui::PopStyleColor(3);
						}

						if (ImGui::BeginPopupContextItem("SceneItemContext"))
						{
							if (isSceneFile && ImGui::MenuItem("Delete"))
							{
								m_PendingDeletePath = entry.path();
								m_OpenDeleteConfirm = true;
							}
							ImGui::EndPopup();
						}

						if (clicked)
						{
							m_SelectedResourcePath = entry.path();
							if (entry.path().extension() == ".json")
							{
								if (m_SceneManager.LoadSceneFromJson(entry.path()))
								{
									m_CurrentScenePath = entry.path();
									m_SelectedObjectName.clear();
									m_LastSelectedObjectName.clear();
									m_ObjectNameBuffer.fill('\0');
								}
							}
						}
						ImGui::PopID();
					}
				}
			};

		drawDirectory(drawDirectory, m_ResourceRoot);
		ImGui::End();
	}


	void EditorApplication::DrawResourceBrowser()
	{
		ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Once);
		ImGui::Begin("Resource Browser");

		if (ImGui::BeginTabBar("ResourceTabs"))
		{
			ImVec2 avail = ImGui::GetContentRegionAvail();
			//Meshes
			if (ImGui::BeginTabItem("Meshes"))
			{
				ImGui::BeginChild("MeshesScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

				const auto& meshes = m_AssetLoader->GetMeshes().GetKeyToHandle();
				for (const auto& [key, handle] : meshes)
				{
					const std::string* displayName = m_AssetLoader->GetMeshes().GetDisplayName(handle);
					const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : key.c_str();
					ImGui::PushID((int)handle.id); // 충돌 방지
					ImGui::Selectable(name);

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload("RESOURCE_MESH", &handle, sizeof(MeshHandle));
						ImGui::Text("Mesh : %s", name);
						ImGui::EndDragDropSource();
					}
					ImGui::Separator();
					ImGui::PopID();
				}

				ImGui::EndChild();
			
				ImGui::EndTabItem();
			}

			//Materials
			if (ImGui::BeginTabItem("Materials"))
			{
				ImGui::BeginChild("MaterialsScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

				const auto& materials = m_AssetLoader->GetMaterials().GetKeyToHandle();
				for (const auto& [key, handle] : materials)
				{
					const std::string* displayName = m_AssetLoader->GetMaterials().GetDisplayName(handle);
					const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : key.c_str();
					ImGui::PushID((int)handle.id); // 충돌 방지
					ImGui::Selectable(name);

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload("RESOURCE_MATERIAL", &handle, sizeof(MaterialHandle));
						ImGui::Text("Material : %s", name);
						ImGui::EndDragDropSource();
					}
					ImGui::Separator(); 
					ImGui::PopID();
				}

				ImGui::EndChild();

				ImGui::EndTabItem();
			}

			//Textures
			if (ImGui::BeginTabItem("Textures"))
			{
				ImGui::BeginChild("TexturesScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

				const auto& textures = m_AssetLoader->GetTextures().GetKeyToHandle();
				for (const auto& [key, handle] : textures)
				{
					const std::string* displayName = m_AssetLoader->GetTextures().GetDisplayName(handle);
					const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : key.c_str();
					ImGui::PushID((int)handle.id); // 충돌 방지
					ImGui::Selectable(name);

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload("RESOURCE_TEXTURE", &handle, sizeof(TextureHandle));
						ImGui::Text("Texture : %s", name);
						ImGui::EndDragDropSource();
					}
					ImGui::Separator();
					ImGui::PopID();
				}

				ImGui::EndChild();

				ImGui::EndTabItem();
			}

			//Skeletons
			if (ImGui::BeginTabItem("Skeletons"))
			{
				ImGui::BeginChild("SkeletonsScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

				const auto& skeletons = m_AssetLoader->GetSkeletons().GetKeyToHandle();
				for (const auto& [key, handle] : skeletons)
				{
					const std::string* displayName = m_AssetLoader->GetSkeletons().GetDisplayName(handle);
					const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : key.c_str();
					ImGui::PushID((int)handle.id); // 충돌 방지
					ImGui::Selectable(name);

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload("RESOURCE_SKELETON", &handle, sizeof(SkeletonHandle));
						ImGui::Text("Skeleton : %s", name);
						ImGui::EndDragDropSource();
					}
					ImGui::Separator();
					ImGui::PopID();
				}

				ImGui::EndChild();

				ImGui::EndTabItem();
			}

			//Animations
			if (ImGui::BeginTabItem("Animations"))
			{
				ImGui::BeginChild("AnimationsScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

				const auto& animations = m_AssetLoader->GetAnimations().GetKeyToHandle();
				for (const auto& [key, handle] : animations)
				{
					const std::string* displayName = m_AssetLoader->GetAnimations().GetDisplayName(handle);
					const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : key.c_str();
					ImGui::PushID((int)handle.id); // 충돌 방지
					ImGui::Selectable(name);

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload("RESOURCE_ANIMATION", &handle, sizeof(AnimationHandle));
						ImGui::Text("Animation : %s", name);
						ImGui::EndDragDropSource();
					}
					ImGui::Separator();
					ImGui::PopID();
				}

				ImGui::EndChild();

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	void EditorApplication::DrawGizmo()
	{
		if (!m_EditorViewport.HasViewportRect())
		{
			return;
		}

		auto scene = m_SceneManager.GetCurrentScene();
		if (!scene)
		{
			return;
		}

		if (m_SelectedObjectName.empty())
		{
			return;
		}

		auto editorCamera = scene->GetEditorCamera().get();
		if (!editorCamera)
		{
			return;
		}

		const auto& opaqueObjects = scene->GetOpaqueObjects();
		const auto& transparentObjects = scene->GetTransparentObjects();

		std::shared_ptr<GameObject> selectedObject;
		if (const auto opaqueIt = opaqueObjects.find(m_SelectedObjectName); opaqueIt != opaqueObjects.end())
		{
			selectedObject = opaqueIt->second;
		}
		else if (const auto transparentIt = transparentObjects.find(m_SelectedObjectName); transparentIt != transparentObjects.end())
		{
			selectedObject = transparentIt->second;
		}

		if (!selectedObject)
		{
			return;
		}

		auto* transform = selectedObject->GetComponent<TransformComponent>();
		if (!transform)
		{
			return;
		}

		static ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
		static ImGuizmo::MODE currentMode = ImGuizmo::WORLD;
		static bool useSnap = false;
		static float snapValues[3]{ 1.0f, 1.0f, 1.0f };

		ImGui::Begin("Gizmo");
		if (ImGui::RadioButton("Translate", currentOperation == ImGuizmo::TRANSLATE))
		{
			currentOperation = ImGuizmo::TRANSLATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", currentOperation == ImGuizmo::ROTATE))
		{
			currentOperation = ImGuizmo::ROTATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", currentOperation == ImGuizmo::SCALE))
		{
			currentOperation = ImGuizmo::SCALE;
		}

		if (currentOperation != ImGuizmo::SCALE)
		{
			if (ImGui::RadioButton("Local", currentMode == ImGuizmo::LOCAL))
			{
				currentMode = ImGuizmo::LOCAL;
			}
			ImGui::SameLine();
			if (ImGui::RadioButton("World", currentMode == ImGuizmo::WORLD))
			{
				currentMode = ImGuizmo::WORLD;
			}
		}
		ImGui::Checkbox("Snap", &useSnap);
		if (useSnap)
		{
			ImGui::InputFloat3("Snap Value", snapValues);
		}
		ImGui::End();

		const ImVec2 rectMin = m_EditorViewport.GetViewportRectMin();
		const ImVec2 rectMax = m_EditorViewport.GetViewportRectMax();
		const ImVec2 rectSize = ImVec2(rectMax.x - rectMin.x, rectMax.y - rectMin.y);

		XMFLOAT4X4 view = editorCamera->GetViewMatrix();
		XMFLOAT4X4 proj = editorCamera->GetProjMatrix();
		XMFLOAT4X4 world = transform->GetWorldMatrix();

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::BeginFrame();
		if (auto* drawList = m_EditorViewport.GetDrawList())
		{
			ImGuizmo::SetDrawlist(drawList);
		}
		ImGuizmo::SetRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y);

		ImGuizmo::Manipulate(
			&view._11,
			&proj._11,
			currentOperation,
			currentMode,
			&world._11,
			nullptr,
			useSnap ? snapValues : nullptr);

		if (ImGuizmo::IsUsing())
		{
			XMFLOAT4X4 local = world;
			if (auto* parent = transform->GetParent())
			{
				local = MathUtils::Mul(world, parent->GetInverseWorldMatrix());
			}

			XMFLOAT3 position{};
			XMFLOAT4 rotation{};
			XMFLOAT3 scale{};
			MathUtils::DecomposeMatrix(local, position, rotation, scale);

			transform->SetPosition(position);
			transform->SetRotation(rotation);
			transform->SetScale(scale);
		}
	}

	void EditorApplication::FocusEditorCameraOnObject(const std::shared_ptr<GameObject>& object)
	{
		if (!object)
		{
			return;
		}

		auto scene = m_SceneManager.GetCurrentScene();
		if (!scene)
		{
			return;
		}

		auto editorCamera = scene->GetEditorCamera();
		if (!editorCamera)
		{
			return;
		}

		auto* cameraComponent = editorCamera->GetComponent<CameraComponent>();
		if (!cameraComponent)
		{
			return;
		}

		auto* transform = object->GetComponent<TransformComponent>();
		if (!transform)
		{
			return;
		}

		const XMFLOAT4X4 world = transform->GetWorldMatrix();
		const XMFLOAT3 target{ world._41, world._42, world._43 };
		const XMFLOAT3 up = XMFLOAT3(0.0f, 0.1f, 0.0f);
		const XMFLOAT3 eye = cameraComponent->GetEye();

		const XMVECTOR eyeVec = XMLoadFloat3(&eye);
		const XMVECTOR targetVec = XMLoadFloat3(&target);
		XMVECTOR toTarget = XMVectorSubtract(targetVec, eyeVec);
		float distance = XMVectorGetX(XMVector3Length(toTarget));
		XMVECTOR forwardVec = XMVectorZero();

		if (distance > 0.00001f)
		{
			forwardVec = XMVector3Normalize(toTarget);
		}
		else
		{
			const XMFLOAT3 look = cameraComponent->GetLook();
			const XMVECTOR lookVec = XMLoadFloat3(&look);
			const XMVECTOR fallback = XMVectorSubtract(lookVec, eyeVec);
			const float fallbackDistance = XMVectorGetX(XMVector3Length(fallback));

			if (fallbackDistance > 0.001f)
			{
				forwardVec = XMVector3Normalize(fallback);
				distance = fallbackDistance;
			}
			else
			{
				forwardVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
				distance = 5.0f;
			}
		}

		const float minDistance = 2.0f;
		const float desiredDistance = (std::max)(minDistance,distance * 0.2f);
		const XMVECTOR newEyeVec = XMVectorSubtract(targetVec, XMVectorScale(forwardVec, desiredDistance));
		XMFLOAT3 newEye{};
		XMStoreFloat3(&newEye, newEyeVec);

		cameraComponent->SetEyeLookUp(newEye, target, up);
	}


	void EditorApplication::CreateDockSpace()
	{
		ImGui::DockSpaceOverViewport(
			ImGui::GetID("EditorDockSpace"),
			ImGui::GetMainViewport(),
			ImGuiDockNodeFlags_PassthruCentralNode
		);
	}

	// 초기 창 셋팅
	void EditorApplication::SetupEditorDockLayout()
	{	
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

		ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.30f, &dockRight, &dockMain); //Right 20%
		ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.2f, &dockBottom, &dockMain);  //Down 20%

		ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Left, 0.40f, &dockRightA, &dockMain); // Right 10%
		ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Right, 0.60f, &dockRightB, &dockMain);// Right 10%
	
		ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.10f, &dockRight, &dockMain);
	
		 //Down 30%

	// 창 할당
		ImGui::DockBuilderDockWindow("Hierarchy", dockRightA);
		ImGui::DockBuilderDockWindow("Inspector", dockRightB);
		ImGui::DockBuilderDockWindow("Folder", dockBottom);
		ImGui::DockBuilderDockWindow("Game", dockMain);
		ImGui::DockBuilderDockWindow("Editor", dockMain);

		ImGui::DockBuilderFinish(dockspaceID);
	}

	void EditorApplication::OnResize(int width, int height)
	{
		__super::OnResize(width, height);
	}

	void EditorApplication::OnClose()
	{
		m_SceneManager.RequestQuit();
	}
