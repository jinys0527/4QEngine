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
#include "InputManager.h"
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
#include "Snapshot.h"
#include "UIManager.h"
#include "UIComponent.h"
#include "UIButtonComponent.h"
#include "UIProgressBarComponent.h"
#include "UIPrimitives.h"
#include "UISliderComponent.h"
#include "UITextComponent.h"
#include <cmath>
#include <functional>
#include <algorithm>
#include <limits>

#define DRAG_SPEED 0.01f
namespace fs = std::filesystem;
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

	m_InputManager = &m_Services.Get<InputManager>();

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
		// Window Message 해석
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			m_InputManager->OnHandleMessage(msg);
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
	ImGuiIO& io = ImGui::GetIO();

	if (m_EditorState == EditorPlayState::Play) {
		return;
	}

	if (!io.WantTextInput)
	{
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
		{
			ClearPendingPropertySnapshots();
			m_UndoManager.Undo();
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
		{
			ClearPendingPropertySnapshots();
			m_UndoManager.Redo();
		}
	}
}

void EditorApplication::Update()
{
	float dTime = m_Engine.GetTime();
	UpdateInput();
	m_SceneManager.StateUpdate(dTime);
	m_SceneManager.Update(dTime);

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


	ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
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
	DrawUIEditorPreview();

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

	//m_Renderer.RenderFrame(m_FrameData, m_SceneRenderTarget, m_SceneRenderTarget_edit);



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

		// ---- 중앙 정렬 계산 ----
		float buttonWidth = 60.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalButtonWidth = buttonWidth * 2 + spacing;

		float windowWidth = ImGui::GetWindowWidth();
		float centerX = (windowWidth - totalButtonWidth) * 0.5f;

		ImGui::SetCursorPosX(centerX);
		//Play = True일 때 비활성

		bool disablePlay = (m_EditorState == EditorPlayState::Play);
		bool disablePause = (m_EditorState != EditorPlayState::Play);
		bool disableStop = (m_EditorState == EditorPlayState::Stop);

		ImGui::BeginDisabled(disablePlay);
		if (ImGui::Button("Play", ImVec2(buttonWidth, 0)))
		{
			m_SceneManager.SaveSceneToJson(m_CurrentScenePath);
			m_SceneManager.GetCurrentScene()->SetIsPause(false);
			m_EditorState = EditorPlayState::Play;


		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::BeginDisabled(disablePause);
		if (ImGui::Button("Pause", ImVec2(buttonWidth, 0)))
		{
			m_SceneManager.GetCurrentScene()->SetIsPause(true);
			m_EditorState = EditorPlayState::Pause;
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::BeginDisabled(disableStop);
		if (ImGui::Button("Stop", ImVec2(buttonWidth, 0)))
		{
			m_SceneManager.LoadSceneFromJson(m_CurrentScenePath);
			m_EditorState = EditorPlayState::Stop;
		}
		ImGui::EndDisabled();

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
			const fs::path oldScenePath = m_CurrentScenePath;
			const fs::path oldSelectedPath = m_SelectedResourcePath;
			fs::path renamedPath = m_CurrentScenePath;
			fs::path renamedSeletedPath = m_SelectedResourcePath;

			if (!m_CurrentScenePath.empty() && m_CurrentScenePath.stem() == oldName)
			{
				renamedPath = m_CurrentScenePath.parent_path() / (newName + m_CurrentScenePath.extension().string());
				if (m_SelectedResourcePath == m_CurrentScenePath)
				{
					renamedSeletedPath = renamedPath;
				}
			}
			m_SelectedResourcePath = renamedSeletedPath;
			m_CurrentScenePath = renamedPath;

			Scene* scenePtr = scene.get();
			m_UndoManager.Push(UndoManager::Command{
				"Rename Scene",
				[this, scenePtr, oldName, oldScenePath, oldSelectedPath]()
				{
					if (!scenePtr)
						return;
					scenePtr->SetName(oldName);
					m_LastSceneName = oldName;
					CopyStringToBuffer(oldName, m_SceneNameBuffer);
					m_CurrentScenePath = oldScenePath;
					m_SelectedResourcePath = oldSelectedPath;
				},
				[this, scenePtr, newName, renamedPath, renamedSeletedPath]()
				{
					if (!scenePtr)
						return;
					scenePtr->SetName(newName);
					m_LastSceneName = newName;
					CopyStringToBuffer(newName, m_SceneNameBuffer);
					m_CurrentScenePath = renamedPath;
					m_SelectedResourcePath = renamedSeletedPath;
				}
				});
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
		auto createdObject = scene->CreateGameObject(name); // GameObject 생성 후 바꾸는 걸로 변경했음
		m_SelectedObjectName = name;

		if (createdObject)
		{
			ObjectSnapshot snapshot;
			createdObject->Serialize(snapshot.data);
			Scene* scenePtr = scene.get();

			m_UndoManager.Push(UndoManager::Command{
				"Create GameObject",
				[scenePtr, snapshot]()
				{
					if (!scenePtr)
						return;

					scenePtr->RemoveGameObjectByName(snapshot.data.value("name", ""));

				},
				[scenePtr, snapshot]()
				{
					ApplySnapshot(scenePtr, snapshot);
				}
				});
		}
	}

	std::unordered_map<std::string, std::shared_ptr<GameObject>> objectLookup;

	auto collectObjects = [&](const auto& objects)
		{
			for (const auto& [name, object] : objects)
			{
				if (!object)
				{
					continue;
				}
				objectLookup[name] = object;
			}
		};

	collectObjects(scene->GetGameObjects());

	auto findObjectByName = [&](const std::string& name) -> std::shared_ptr<GameObject>
		{
			if (const auto it = objectLookup.find(name); it != objectLookup.end())
			{
				return it->second;
			}
			return nullptr;
		};

	// copy
	const std::shared_ptr<GameObject>* selectedObject = nullptr;
	if (const auto it = objectLookup.find(m_SelectedObjectName); it != objectLookup.end())
	{
		selectedObject = &it->second;
	}

	auto copySelectedObject = [&](const std::shared_ptr<GameObject>& object)
		{
			if (!object)
			{
				return;
			}

			nlohmann::json clipboard = nlohmann::json::object();
			clipboard["objects"] = nlohmann::json::array();

			std::unordered_map<GameObject*, int> objectIds;
			int nextId = 0;
			std::string rootParentName;
			if (auto* rootTransform = object->GetComponent<TransformComponent>())
			{
				if (auto* rootParent = rootTransform->GetParent())
				{
					if (auto* rootParentOwner = dynamic_cast<GameObject*>(rootParent->GetOwner()))
					{
						rootParentName = rootParentOwner->GetName();
					}
				}
			}
			std::vector<GameObject*> stack;
			stack.push_back(object.get());
			while (!stack.empty())
			{
				GameObject* current = stack.back();
				stack.pop_back();

				if (!current)
				{
					continue;
				}

				const bool isRoot = (current == object.get());
				auto* currentTransform = current->GetComponent<TransformComponent>();
				GameObject* parentObject = (!isRoot && currentTransform && currentTransform->GetParent())
					? dynamic_cast<GameObject*>(currentTransform->GetParent()->GetOwner())
					: nullptr;

				if (objectIds.find(current) == objectIds.end())
				{
					objectIds[current] = nextId++;
				}

				const int currentId = objectIds[current];
				int parentId = -1;
				if (parentObject)
				{
					if (objectIds.find(parentObject) == objectIds.end())
					{
						objectIds[parentObject] = nextId++;
					}
					parentId = objectIds[parentObject];
				}

				nlohmann::json objectJson;
				current->Serialize(objectJson);
				objectJson["clipboardId"] = currentId;
				objectJson["parentId"] = parentId;
        
				if (isRoot && !rootParentName.empty())
				{
					objectJson["externalParentName"] = rootParentName;
				}
				/*const auto opacityIt = objectOpacity.find(current);
				objectJson["isOpaque"] = (opacityIt != objectOpacity.end()) ? opacityIt->second : true;*/

				clipboard["objects"].push_back(std::move(objectJson));

				if (!currentTransform)
				{
					continue;
				}

				for (auto* childTransform : currentTransform->GetChildrens())
				{
					if (!childTransform)
					{
						continue;
					}
					auto* childObject = dynamic_cast<GameObject*>(childTransform->GetOwner());
					if (childObject)
					{
						stack.push_back(childObject);
					}
				}
			}

			m_ObjectClipboard = std::move(clipboard);
			m_ObjectClipboardHasData = true;
		};

	struct PendingAdd
	{
		nlohmann::json data;
		std::string label;
	};

	std::vector<PendingAdd> pendingAdds;

	auto queuePasteObject = [&](nlohmann::json objectJson, std::string label)
		{
			pendingAdds.push_back(PendingAdd{ std::move(objectJson), std::move(label) });
		};

	auto pasteClipboardObject = [&]()
		{
			if (!m_ObjectClipboardHasData)
			{
				return false;
			}
			if (!m_ObjectClipboard.is_object())
			{
				m_ObjectClipboardHasData = false;
				return false;
			}
			queuePasteObject(m_ObjectClipboard, "Paste GameObject");
			return true;
		};

	std::vector<std::string> pendingDeletes;



	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C) && selectedObject && *selectedObject)
		{
			copySelectedObject(*selectedObject);
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
		{
			pasteClipboardObject();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Delete) && selectedObject && *selectedObject)
		{
			pendingDeletes.push_back((*selectedObject)->GetName());
		}
	}


	ImGui::Separator();

	if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (!m_ObjectClipboardHasData)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::MenuItem("Paste"))
		{
			pasteClipboardObject();
		}
		if (!m_ObjectClipboardHasData)
		{
			ImGui::EndDisabled();
		}
		ImGui::EndPopup();
	}


	auto isDescendant = [&](TransformComponent* child, TransformComponent* potentialParent) -> bool
		{
			for (auto* current = potentialParent; current != nullptr; current = current->GetParent())
			{
				if (current == child)
				{
					return true;
				}
			}
			return false;
		};

	auto reparentObject = [&](const std::string& childName, const std::string& parentName) {
		if (childName == parentName) {
			return;
		}
		auto childObject = findObjectByName(childName);
		auto parentObject = findObjectByName(parentName);

		if (!childObject || !parentObject)
		{
			return;
		}

		auto* childTransform = childObject->GetComponent<TransformComponent>();
		auto* parentTransform = parentObject->GetComponent<TransformComponent>();
		if (!childTransform || !parentTransform)
		{
			return;
		}
		if (childTransform->GetParent() == parentTransform)
		{
			return;
		}
		if (isDescendant(childTransform, parentTransform))
		{
			return;
		}

		SceneStateSnapshot beforeState = CaptureSceneState(scene);

		if (childTransform->GetParent())
		{
			childTransform->DetachFromParentKeepWorld();
		}
		childTransform->SetParentKeepWorld(parentTransform);

		SceneStateSnapshot afterState = CaptureSceneState(scene);
		m_UndoManager.Push(UndoManager::Command{
			"Reparent GameObject",
			[this, beforeState]()
			{
				RestoreSceneState(beforeState);
			},
			[this, afterState]()
			{
				RestoreSceneState(afterState);
			}
			});
		};

	auto detachObject = [&](const std::string& childName)
		{
			auto childObject = findObjectByName(childName);
			if (!childObject)
			{
				return;
			}

			auto* childTransform = childObject->GetComponent<TransformComponent>();
			if (!childTransform || !childTransform->GetParent())
			{
				return;
			}

			SceneStateSnapshot beforeState = CaptureSceneState(scene);
			childTransform->DetachFromParentKeepWorld();
			SceneStateSnapshot afterState = CaptureSceneState(scene);
			m_UndoManager.Push(UndoManager::Command{
				"Detach GameObject",
				[this, beforeState]()
				{
					RestoreSceneState(beforeState);
				},
				[this, afterState]()
				{
					RestoreSceneState(afterState);
				}
				});
		};


	std::vector<GameObject*> rootObjects;
	rootObjects.reserve(objectLookup.size());
	for (const auto& [name, object] : objectLookup)
	{
		auto* transform = object->GetComponent<TransformComponent>();
		if (!transform || transform->GetParent() == nullptr)
		{
			rootObjects.push_back(object.get());
		}
	}
	std::sort(rootObjects.begin(), rootObjects.end(),
		[](const GameObject* a, const GameObject* b)
		{
			if (!a || !b)
			{
				return a < b;
			}
			return a->GetName() < b->GetName();
		});
	auto drawHierarchyNode = [&](auto&& self, GameObject* object) -> void
		{
			if (!object) { return; }
			const std::string& name = object->GetName();
			auto* transform = object->GetComponent<TransformComponent>();
			auto* children = transform ? &transform->GetChildrens() : nullptr;
			const bool hasChildren = children && !children->empty();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
			if (m_SelectedObjectName == name)
			{
				flags |= ImGuiTreeNodeFlags_Selected;
			}

			ImGui::PushID(object);

			if (!hasChildren) {
				flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

				ImGui::TreeNodeEx(name.c_str(), flags);
				if (ImGui::IsItemClicked())
				{
					m_SelectedObjectName = name;
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_SelectedObjectName = name;
					auto selected = findObjectByName(name);
					if (selected)
					{
						FocusEditorCameraOnObject(selected);
					}
				}

				if (ImGui::BeginPopupContextItem("ObjectContext"))
				{
					auto selected = findObjectByName(name);
					if (ImGui::MenuItem("Copy") && selected)
					{
						copySelectedObject(selected);
					}
					if (ImGui::MenuItem("Duplicate") && selected)
					{
						copySelectedObject(selected);
						if (m_ObjectClipboardHasData)
						{
							queuePasteObject(m_ObjectClipboard, "Duplicate GameObject");
						}
					}
					if (ImGui::MenuItem("Delete"))
					{
						pendingDeletes.push_back(name);
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("HIERARCHY_OBJECT", name.c_str(), name.size() + 1);
					ImGui::TextUnformatted(name.c_str());
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECT"))
					{
						const char* payloadName = static_cast<const char*>(payload->Data);
						if (payloadName)
						{
							reparentObject(payloadName, name);
						}
					}
					ImGui::EndDragDropTarget();
				}
			}
			else {
				const bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), flags);
				if (ImGui::IsItemClicked())
				{
					m_SelectedObjectName = name;
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_SelectedObjectName = name;
					auto selected = findObjectByName(name);
					if (selected)
					{
						FocusEditorCameraOnObject(selected);
					}
				}
				if (ImGui::BeginPopupContextItem("ObjectContext"))
				{
					auto selected = findObjectByName(name);
					if (ImGui::MenuItem("Copy") && selected)
					{
						copySelectedObject(selected);
					}
					if (ImGui::MenuItem("Duplicate") && selected)
					{
						copySelectedObject(selected);
						if (m_ObjectClipboardHasData)
						{
							queuePasteObject(m_ObjectClipboard, "Duplicate GameObject");
						}
					}
					if (ImGui::MenuItem("Delete"))
					{
						pendingDeletes.push_back(name);
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("HIERARCHY_OBJECT", name.c_str(), name.size() + 1);
					ImGui::TextUnformatted(name.c_str());
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECT"))
					{
						const char* payloadName = static_cast<const char*>(payload->Data);
						if (payloadName)
						{
							reparentObject(payloadName, name);
						}
					}
					ImGui::EndDragDropTarget();
				}

				if (nodeOpen)
				{
					for (auto* childTransform : *children)
					{
						if (!childTransform)
						{
							continue;
						}
						auto* childOwner = dynamic_cast<GameObject*>(childTransform->GetOwner());
						if (!childOwner)
						{
							continue;
						}
						self(self, childOwner);
					}
					ImGui::TreePop();
				}
			}

			ImGui::PopID();
		};

	for (auto* root : rootObjects)
	{
		drawHierarchyNode(drawHierarchyNode, root);
	}

	const ImVec2 dropRegion = ImGui::GetContentRegionAvail();
	if (dropRegion.x > 0.0f && dropRegion.y > 0.0f)
	{
		ImGui::InvisibleButton("##HierarchyDropTarget", dropRegion);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECT"))
			{
				const char* payloadName = static_cast<const char*>(payload->Data);
				if (payloadName)
				{
					detachObject(payloadName);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	for (auto& pendingAdd : pendingAdds)
	{
		bool didAdd = false;
		SceneStateSnapshot beforeState = CaptureSceneState(scene);

		if (pendingAdd.data.contains("objects") && pendingAdd.data["objects"].is_array())
		{
			std::unordered_map<int, std::shared_ptr<GameObject>> createdObjects;

			for (auto& objectJson : pendingAdd.data["objects"])
			{
				const int clipboardId = objectJson.value("clipboardId", static_cast<int>(createdObjects.size()));
				const std::string baseName = objectJson.value("name", "GameObject");
				const std::string uniqueName = MakeUniqueObjectName(*scene, baseName);

				objectJson["name"] = uniqueName;
				auto newObject = std::make_shared<GameObject>(scene->GetEventDispatcher());
				newObject->Deserialize(objectJson);
				scene->AddGameObject(newObject);
				createdObjects[clipboardId] = newObject;
				m_SelectedObjectName = uniqueName;
				didAdd = true;
			}
			for (const auto& objectJson : pendingAdd.data["objects"])
				{
				const int clipboardId = objectJson.value("clipboardId", -1);
				const int parentId = objectJson.value("parentId", -1);
				if (clipboardId < 0)
				{
					continue;
				}

				auto childIt = createdObjects.find(clipboardId);
				if (childIt == createdObjects.end())
				{
					continue;
				}

				auto* childTransform = childIt->second->GetComponent<TransformComponent>();
				if (!childTransform)
				{
					continue;
				}

				TransformComponent* parentTransform = nullptr;
				if (parentId >= 0)
				{
					auto parentIt = createdObjects.find(parentId);
					if (parentIt != createdObjects.end())
					{
						parentTransform = parentIt->second->GetComponent<TransformComponent>();
					}
				}
				if (!parentTransform)
				{
					const std::string parentName = objectJson.value("externalParentName", "");
					if (!parentName.empty())
					{
						auto parentObject = findObjectByName(parentName);
						if (parentObject)
						{
							parentTransform = parentObject->GetComponent<TransformComponent>();
						}
					}
				}
				if (!parentTransform)
				{
					continue;
				}

				if (childTransform->GetParent())
				{
					childTransform->DetachFromParentKeepLocal();
				}
				childTransform->SetParentKeepLocal(parentTransform);
			}
				
		}
		else if (pendingAdd.data.contains("components") && pendingAdd.data["components"].is_array())
		{
			const std::string baseName = pendingAdd.data.value("name", "GameObject");
			const std::string uniqueName = MakeUniqueObjectName(*scene, baseName);
			pendingAdd.data["name"] = uniqueName;

			auto newObject = std::make_shared<GameObject>(scene->GetEventDispatcher());
			newObject->Deserialize(pendingAdd.data);
			scene->AddGameObject(newObject);
			m_SelectedObjectName = uniqueName;
			didAdd = true;
		}

		if (didAdd)
		{
			SceneStateSnapshot afterState = CaptureSceneState(scene);
			m_UndoManager.Push(UndoManager::Command{
				pendingAdd.label,
				[this, beforeState]()
				{
					RestoreSceneState(beforeState);
				},
				[this, afterState]()
				{
					RestoreSceneState(afterState);
				}
				});
		}
	}

	for (const auto& name : pendingDeletes)
	{
		SceneStateSnapshot beforeState = CaptureSceneState(scene);
		scene->RemoveGameObjectByName(name);
		if (m_SelectedObjectName == name)
		{
			m_SelectedObjectName.clear();
		}
		SceneStateSnapshot afterState = CaptureSceneState(scene);

		m_UndoManager.Push(UndoManager::Command{
			"Delete GameObject",
			[this, beforeState]()
			{
				RestoreSceneState(beforeState);
			},
			[this, afterState]()
			{
				RestoreSceneState(afterState);
			}
			});
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
	const auto  it = gameObjects.find(m_SelectedObjectName);


	// 선택된 오브젝트가 없거나, 실체가 없는 경우
	//second == Object 포인터
	if (it == gameObjects.end() || !it->second)
	{
		ImGui::Text("No Selected GameObject");
		ImGui::End();
		return;
	}

	auto selectedObject = it->second;

	if (m_LastPendingSnapshotScenePath != m_CurrentScenePath)
	{
		m_PendingPropertySnapshots.clear();
		m_LastPendingSnapshotScenePath = m_CurrentScenePath;
	}

	if (m_LastSelectedObjectName != m_SelectedObjectName)
	{
		m_PendingPropertySnapshots.clear();
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
			const std::string oldName = it->second->GetName();
			if (scene->RenameGameObject(oldName, newName))
			{
				m_SelectedObjectName = newName;
				m_LastSelectedObjectName = newName;

				Scene* scenePtr = scene.get();
				m_UndoManager.Push(UndoManager::Command{
					"Rename GameObject",
					[this, scenePtr, oldName, newName]()
					{
						if (!scenePtr)
							return;
						scenePtr->RenameGameObject(newName, oldName);
						m_SelectedObjectName = oldName;
						m_LastSelectedObjectName = oldName;
						CopyStringToBuffer(oldName, m_ObjectNameBuffer);
					},
					[this, scenePtr, oldName, newName]()
					{
						if (!scenePtr)
							return;
						scenePtr->RenameGameObject(oldName, newName);
						m_SelectedObjectName = newName;
						m_LastSelectedObjectName = newName;
						CopyStringToBuffer(newName, m_ObjectNameBuffer);
					}
					});
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
				ObjectSnapshot beforeSnapshot;
				selectedObject->Serialize(beforeSnapshot.data);

				if (selectedObject->RemoveComponentByTypeName(typeName))
				{
					ObjectSnapshot afterSnapshot;
					selectedObject->Serialize(afterSnapshot.data);

					Scene* scenePtr = scene.get();
					const std::string label = "Remove Component " + typeName;
					m_UndoManager.Push(UndoManager::Command{
						label,
						[scenePtr, beforeSnapshot]()
						{
							ApplySnapshot(scenePtr, beforeSnapshot);
						},
						[scenePtr, afterSnapshot]()
						{
							ApplySnapshot(scenePtr, afterSnapshot);
						}
						});
				}
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
					const PropertyEditResult editResult = DrawComponentPropertyEditor(component, *prop, *m_AssetLoader);
					const bool updated = editResult.updated;
					const bool activated = editResult.activated;
					const bool deactivated = editResult.deactivated;
					const size_t propertyKey = MakePropertyKey(component, prop->GetName());

					if (activated && m_PendingPropertySnapshots.find(propertyKey) == m_PendingPropertySnapshots.end())
					{
						PendingPropertySnapshot pendingSnapshot;
						selectedObject->Serialize(pendingSnapshot.beforeSnapshot.data);
						m_PendingPropertySnapshots.emplace(propertyKey, std::move(pendingSnapshot));
					}

					if (updated)
					{
						auto itSnapshot = m_PendingPropertySnapshots.find(propertyKey);
						if (itSnapshot != m_PendingPropertySnapshots.end())
						{
							itSnapshot->second.updated = true;
						}
					}

					if (deactivated)
					{
						auto itSnapshot = m_PendingPropertySnapshots.find(propertyKey);
						if (itSnapshot != m_PendingPropertySnapshots.end())
						{
							if (itSnapshot->second.updated)
							{
								ObjectSnapshot beforeSnapshot = itSnapshot->second.beforeSnapshot;
								ObjectSnapshot afterSnapshot;
								selectedObject->Serialize(afterSnapshot.data);

								Scene* scenePtr = scene.get();
								const std::string label = "Edit " + prop->GetName();
								m_UndoManager.Push(UndoManager::Command{
									label,
									[scenePtr, beforeSnapshot]()
									{
										ApplySnapshot(scenePtr, beforeSnapshot);
									},
									[scenePtr, afterSnapshot]()
									{
										ApplySnapshot(scenePtr, afterSnapshot);
									}
									});
							}
							m_PendingPropertySnapshots.erase(itSnapshot);
						}
					}
				}

				if (auto* meshComponent = dynamic_cast<MeshComponent*>(component))
				{
					ObjectSnapshot beforeSnapshot;
					selectedObject->Serialize(beforeSnapshot.data);

					const bool updated = DrawSubMeshOverridesEditor(*meshComponent, *m_AssetLoader);

					if (updated)
					{
						ObjectSnapshot afterSnapshot;
						selectedObject->Serialize(afterSnapshot.data);

						Scene* scenePtr = scene.get();
						m_UndoManager.Push(UndoManager::Command{
							"Edit SubMesh Overrides",
							[scenePtr, beforeSnapshot]()
							{
								ApplySnapshot(scenePtr, beforeSnapshot);
							},
							[scenePtr, afterSnapshot]()
							{
								ApplySnapshot(scenePtr, afterSnapshot);
							}
							});
					}
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
			if (typeName == "LightComponent")
			{
				continue;
			}

			const bool hasType = std::find(existingTypes.begin(), existingTypes.end(), typeName) != existingTypes.end();
			//const bool disallowDuplicate = (typeName == "TransformComponent");
			const bool disabled = hasType /*&& disallowDuplicate*/;

			if (disabled)
			{
				ImGui::BeginDisabled();
			}

			if (ImGui::MenuItem(typeName.c_str()))
			{
				ObjectSnapshot beforeSnapshot;
				selectedObject->Serialize(beforeSnapshot.data);

				auto comp = ComponentFactory::Instance().Create(typeName);
				if (comp)
				{
					selectedObject->AddComponent(std::move(comp));

					ObjectSnapshot afterSnapshot;
					selectedObject->Serialize(afterSnapshot.data);

					Scene* scenePtr = scene.get();
					const std::string label = "Add Component " + typeName;
					m_UndoManager.Push(UndoManager::Command{
						label,
						[scenePtr, beforeSnapshot]()
						{
							ApplySnapshot(scenePtr, beforeSnapshot);
						},
						[scenePtr, afterSnapshot]()
						{
							ApplySnapshot(scenePtr, afterSnapshot);
						}
						});
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

	// new Scene 생성 -> Directional light / Main Camera Default 생성
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

			SceneStateSnapshot beforeState = CaptureSceneState(m_SceneManager.GetCurrentScene());
			SceneFileSnapshot beforeFile = CaptureFileSnapshot(newPath);

			if (m_SceneManager.CreateNewScene(newPath))
			{
				m_CurrentScenePath = newPath;
				m_SelectedResourcePath = newPath;
				m_SelectedObjectName.clear();
				m_LastSelectedObjectName.clear();
				m_ObjectNameBuffer.fill('\0');

				SceneStateSnapshot afterState = CaptureSceneState(m_SceneManager.GetCurrentScene());
				SceneFileSnapshot  afterFile = CaptureFileSnapshot(newPath);

				m_UndoManager.Push(UndoManager::Command{
					"Create Scene",
					[this, beforeState, beforeFile]()
					{
						RestoreSceneState(beforeState);
						RestoreFileSnapshot(beforeFile);
					},
					[this, afterState, afterFile]()
					{
						RestoreSceneState(afterState);
						RestoreFileSnapshot(afterFile);
					}
					});
			}
		};
	ImGui::BeginDisabled(m_EditorState == EditorPlayState::Play || m_EditorState == EditorPlayState::Pause);
	if (ImGui::Button("New Scene"))
	{
		createNewScene();
	}
	//ImGui::EndDisabled();

	ImGui::SameLine();
	// Play 중일때 저장 불가
	//ImGui::BeginDisabled(m_EditorState == EditorPlayState::Play || m_EditorState == EditorPlayState::Pause); 
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
			const fs::path beforeCurrentPath = m_CurrentScenePath;
			const fs::path beforeSelectedPath = m_SelectedResourcePath;
			SceneFileSnapshot beforeFile = CaptureFileSnapshot(m_PendingSavePath);

			if (m_SceneManager.SaveSceneToJson(m_PendingSavePath))
			{
				m_CurrentScenePath = m_PendingSavePath;
				m_SelectedResourcePath = m_PendingSavePath;

				const fs::path afterCurrentPath = m_CurrentScenePath;
				const fs::path afterSelectedPath = m_SelectedResourcePath;
				SceneFileSnapshot afterFile = CaptureFileSnapshot(m_PendingSavePath);

				m_UndoManager.Push(UndoManager::Command{
					"Save Scene",
					[this, beforeCurrentPath, beforeSelectedPath, beforeFile]()
					{
						RestoreFileSnapshot(beforeFile);
						m_CurrentScenePath = beforeCurrentPath;
						m_SelectedResourcePath = beforeSelectedPath;
					},
					[this, afterCurrentPath, afterSelectedPath, afterFile]()
					{
						RestoreFileSnapshot(afterFile);
						m_CurrentScenePath = afterCurrentPath;
						m_SelectedResourcePath = afterSelectedPath;
					}
					});
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
			const fs::path targetPath = m_PendingDeletePath;
			const bool wasCurrent = (m_CurrentScenePath == targetPath);
			const bool wasSelected = (m_SelectedResourcePath == targetPath);
			SceneFileSnapshot beforeFile = CaptureFileSnapshot(targetPath);

			std::error_code error;
			std::filesystem::remove(targetPath, error);
			if (!error)
			{
				if (wasCurrent)
				{
					m_CurrentScenePath.clear();
				}
				if (wasSelected)
				{
					m_SelectedResourcePath.clear();
				}
			}

			SceneFileSnapshot afterFile;
			afterFile.path = targetPath;
			afterFile.existed = false;

			m_UndoManager.Push(UndoManager::Command{
				"Delete Scene File",
				[this, beforeFile, wasCurrent, wasSelected]()
				{
					RestoreFileSnapshot(beforeFile);
					if (wasCurrent)
					{
						m_CurrentScenePath = beforeFile.path;
					}
					if (wasSelected)
					{
						m_SelectedResourcePath = beforeFile.path;
					}
				},
				[this, afterFile, wasCurrent, wasSelected]()
				{
					RestoreFileSnapshot(afterFile);
					if (wasCurrent)
					{
						m_CurrentScenePath = afterFile.path;
					}
					if (wasSelected)
					{
						m_SelectedResourcePath = afterFile.path;
					}
				}
				});

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
						SceneStateSnapshot beforeState = CaptureSceneState(m_SceneManager.GetCurrentScene());
						m_SelectedResourcePath = entry.path();
						if (entry.path().extension() == ".json")
						{
							if (m_SceneManager.LoadSceneFromJson(entry.path()))
							{
								m_CurrentScenePath = entry.path();
								m_SelectedObjectName.clear();
								m_LastSelectedObjectName.clear();
								m_ObjectNameBuffer.fill('\0');

								SceneStateSnapshot afterState = CaptureSceneState(m_SceneManager.GetCurrentScene());

								m_UndoManager.Push(UndoManager::Command{
									"Load Scene",
									[this, beforeState]()
									{
										if (beforeState.hasScene)
										{
											m_SceneManager.LoadSceneFromJsonData(beforeState.data, beforeState.currentPath);
										}
										m_CurrentScenePath = beforeState.currentPath;
										m_SelectedResourcePath = beforeState.selectedPath;
										m_SelectedObjectName = beforeState.selectedObjectName;
										m_LastSelectedObjectName = beforeState.lastSelectedObjectName;
										m_ObjectNameBuffer = beforeState.objectNameBuffer;
										m_LastSceneName = beforeState.lastSceneName;
										m_SceneNameBuffer = beforeState.sceneNameBuffer;
									},
									[this, afterState]()
									{
										if (afterState.hasScene)
										{
											m_SceneManager.LoadSceneFromJsonData(afterState.data, afterState.currentPath);
										}
										m_CurrentScenePath = afterState.currentPath;
										m_SelectedResourcePath = afterState.selectedPath;
										m_SelectedObjectName = afterState.selectedObjectName;
										m_LastSelectedObjectName = afterState.lastSelectedObjectName;
										m_ObjectNameBuffer = afterState.objectNameBuffer;
										m_LastSceneName = afterState.lastSceneName;
										m_SceneNameBuffer = afterState.sceneNameBuffer;
									}
									});
							}
						}
						
					}
					ImGui::PopID();
				}
			}
			ImGui::EndDisabled();
		};

	drawDirectory(drawDirectory, m_ResourceRoot);
	ImGui::End();
}


void EditorApplication::DrawResourceBrowser()
{
	ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Once);
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


		//Vertex Shaders
		if (ImGui::BeginTabItem("Vertex Shaders"))
		{
			ImGui::BeginChild("VertexShadersScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto& shaders = m_AssetLoader->GetVertexShaders().GetKeyToHandle();
			for (const auto& [key, handle] : shaders)
			{
				const std::string* displayName = m_AssetLoader->GetVertexShaders().GetDisplayName(handle);
				const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : key.c_str();
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name);

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_VERTEX_SHADER", &handle, sizeof(VertexShaderHandle));
					ImGui::Text("Vertex Shader : %s", name);
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		//Pixel Shaders
		if (ImGui::BeginTabItem("Pixel Shaders"))
		{
			ImGui::BeginChild("PixelShadersScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto& shaders = m_AssetLoader->GetPixelShaders().GetKeyToHandle();
			for (const auto& [key, handle] : shaders)
			{
				const std::string* displayName = m_AssetLoader->GetPixelShaders().GetDisplayName(handle);
				const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : key.c_str();
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name);

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_PIXEL_SHADER", &handle, sizeof(PixelShaderHandle));
					ImGui::Text("Pixel Shader : %s", name);
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		//Shader Assets
		if (ImGui::BeginTabItem("Shader Assets"))
		{
			static char shaderAssetName[128] = "";
			static char shaderVertexPath[256] = "";
			static char shaderPixelPath[256] = "";

			if (ImGui::Button("New Shader Asset"))
			{
				ImGui::OpenPopup("CreateShaderAssetPopup");
			}

			if (ImGui::BeginPopup("CreateShaderAssetPopup"))
			{
				ImGui::InputText("Name", shaderAssetName, sizeof(shaderAssetName));
				ImGui::InputText("VS Path", shaderVertexPath, sizeof(shaderVertexPath));
				ImGui::InputText("PS Path", shaderPixelPath, sizeof(shaderPixelPath));

				if (ImGui::Button("Create"))
				{
					const fs::path resourceRoot = "../ResourceOutput";
					const fs::path assetDir = resourceRoot / "ShaderAsset";
					const fs::path metaDir = assetDir / "Meta";
					const fs::path metaPath = metaDir / (std::string(shaderAssetName) + ".shader.json");
					// “실제 shader 파일들이 있는 폴더(절대/상대 상관없음, 하지만 path로 관리)”
					const fs::path shaderDirAbsOrRel = "../MRenderer/fx/";

					// meta 기준 상대경로 prefix 생성 (Meta -> shaderDir)
					const fs::path shaderDirFromMeta = fs::relative(shaderDirAbsOrRel, metaDir).lexically_normal();

					// 최종 vs/ps 경로(메타 기준 상대경로)
					const fs::path vsRel = (shaderDirFromMeta / shaderVertexPath).lexically_normal();
					const fs::path psRel = (shaderDirFromMeta / shaderPixelPath).lexically_normal();

					if (!shaderAssetName[0])
					{
						ImGui::CloseCurrentPopup();
					}
					else
					{
						auto makeShaderPath = [&](const char* input, const char* fallbackSuffix) -> std::string
							{
								std::string path = input && input[0] ? input : "";
								if (path.empty())
								{
									path = std::string(shaderAssetName) + fallbackSuffix;
								}

								const fs::path parsed = fs::path(path);
								if (parsed.extension().empty())
								{
									return (parsed.string() + ".hlsl");
								}
								return parsed.string();
							};
						fs::create_directories(metaDir);
						json meta = json::object();
						meta["name"] = shaderAssetName;
						meta["vs"] = makeShaderPath(vsRel.generic_string().c_str(), "_VS");
						meta["ps"] = makeShaderPath(psRel.generic_string().c_str(), "_PS");

						std::ofstream out(metaPath);
						if (out)
						{
							out << meta.dump(4);
							out.close();
							if (m_AssetLoader)
							{
								m_AssetLoader->LoadShaderAsset(metaPath.string());
							}
						}
						shaderAssetName[0] = '\0';
						shaderVertexPath[0] = '\0';
						shaderPixelPath[0] = '\0';
						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::Separator();

			ImGui::BeginChild("ShaderAssetsScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto& shaderAssets = m_AssetLoader->GetShaderAssets().GetKeyToHandle();
			for (const auto& [key, handle] : shaderAssets)
			{
				const std::string* displayName = m_AssetLoader->GetShaderAssets().GetDisplayName(handle);
				const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : key.c_str();
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name);

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_SHADER_ASSET", &handle, sizeof(ShaderAssetHandle));
					ImGui::Text("Shader Asset : %s", name);
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

	const auto& gameObjects = scene->GetGameObjects();

	std::shared_ptr<GameObject> selectedObject;
	if (const auto it = gameObjects.find(m_SelectedObjectName); it != gameObjects.end())
	{
		selectedObject = it->second;
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
	if (ImGui::RadioButton("Translate", currentOperation == ImGuizmo::TRANSLATE) || ImGui::IsKeyPressed(ImGuiKey_T))
	{
		currentOperation = ImGuizmo::TRANSLATE;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", currentOperation == ImGuizmo::ROTATE) || ImGui::IsKeyPressed(ImGuiKey_R))
	{
		currentOperation = ImGuizmo::ROTATE;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", currentOperation == ImGuizmo::SCALE) || ImGui::IsKeyPressed(ImGuiKey_S))
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

	static bool wasUsing = false;
	static bool gizmoUpdated = false;
	static bool hasSnapshot = false;
	static ObjectSnapshot beforeSnapshot{};
	static std::string pendingObjectName;

	const bool usingNow = ImGuizmo::IsUsing();

	if (usingNow && !wasUsing)
	{
		selectedObject->Serialize(beforeSnapshot.data);
		pendingObjectName = selectedObject->GetName();
		gizmoUpdated = false;
		hasSnapshot = true;
	}

	if (usingNow)
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
		gizmoUpdated = true;
	}

	if (!usingNow && wasUsing && hasSnapshot && gizmoUpdated)
	{
		if (auto targetObject = FindSceneObject(scene.get(), pendingObjectName))
		{
			ObjectSnapshot afterSnapshot;
			targetObject->Serialize(afterSnapshot.data);

			const char* opLabel = "Gizmo Edit";
			switch (currentOperation)
			{
			case ImGuizmo::TRANSLATE:
				opLabel = "Gizmo Translate";
				break;
			case ImGuizmo::ROTATE:
				opLabel = "Gizmo Rotate";
				break;
			case ImGuizmo::SCALE:
				opLabel = "Gizmo Scale";
				break;
			default:
				break;
			}

			Scene* scenePtr = scene.get();
			ObjectSnapshot undoSnapshot = beforeSnapshot;
			m_UndoManager.Push(UndoManager::Command{
				opLabel,
				[scenePtr, undoSnapshot]()
				{
					ApplySnapshot(scenePtr, undoSnapshot);
				},
				[scenePtr, afterSnapshot]()
				{
					ApplySnapshot(scenePtr, afterSnapshot);
				}
				});
		}
	}

	if (!usingNow && wasUsing)
	{
		hasSnapshot = false;
		gizmoUpdated = false;
		pendingObjectName.clear();
	}

	wasUsing = usingNow;
}

void EditorApplication::DrawUIEditorPreview()
{
	ImGui::Begin("UI Editor");
	ImGui::TextDisabled("UI Editor Preview");
	ImGui::Separator();

	ImGui::Columns(2, "UIDemoColumns", true);

	ImGui::BeginChild("WidgetTreePanel", ImVec2(0, 0), true);
	ImGui::Text("UI Objects");
	ImGui::Separator();

	auto scene = m_SceneManager.GetCurrentScene();
	UIManager* uiManager = m_SceneManager.GetUIManager();
	if (!scene || !uiManager)
	{
		ImGui::TextDisabled("UIManager Or Scene Not Ready.");
	}
	else
	{
		const auto& uiObjectsByScene = uiManager->GetUIObjects();
		const auto sceneName = scene->GetName();
		const auto it = uiObjectsByScene.find(sceneName);

		auto getUIObjectByName = [&](const std::string& name) -> std::shared_ptr<UIObject>
			{
				if (it == uiObjectsByScene.end())
				{
					return nullptr;
				}

				const auto found = it->second.find(name);
				if (found == it->second.end())
				{
					return nullptr;
				}

				return found->second;
			};

		auto pushUISnapshotUndo = [&](const std::string& label, const nlohmann::json& beforeSnapshot, const nlohmann::json& afterSnapshot)
			{
				m_UndoManager.Push(UndoManager::Command{
					label,
					[this, uiManager, sceneName, beforeSnapshot]()
					{
						if (!uiManager)
							return;

						auto& map = uiManager->GetUIObjects();
						auto itScene = map.find(sceneName);
						if (itScene == map.end())
							return;

						const std::string& name = beforeSnapshot.value("name", "");
						auto itObj = itScene->second.find(name);
						if (itObj != itScene->second.end() && itObj->second)
						{
							itObj->second->DeSerialize(beforeSnapshot);
							itObj->second->UpdateInteractableFlags();
						}
					},
					[this, uiManager, sceneName, afterSnapshot]()
					{
						if (!uiManager)
							return;

						auto& map = uiManager->GetUIObjects();
						auto itScene = map.find(sceneName);
						if (itScene == map.end())
							return;

						const std::string& name = afterSnapshot.value("name", "");
						auto itObj = itScene->second.find(name);
						if (itObj != itScene->second.end() && itObj->second)
						{
							itObj->second->DeSerialize(afterSnapshot);
							itObj->second->UpdateInteractableFlags();
						}
					}
					});
			};

		if (!m_SelectedUIObjectName.empty() && !getUIObjectByName(m_SelectedUIObjectName))
		{
			m_SelectedUIObjectName.clear();
		}

		if (ImGui::Button("Add UI Object"))
		{
			std::string baseName = "UIObject";
			std::string name = baseName;
			int suffix = 1;
			if (it != uiObjectsByScene.end())
			{
				while (it->second.find(name) != it->second.end())
				{
					name = baseName + std::to_string(suffix++);
				}
			}

			auto uiObject = std::make_shared<UIObject>(scene->GetEventDispatcher());
			uiObject->SetName(name);
			uiObject->SetBounds(UIRect{ 20.0f, 20.0f, 200.0f, 80.0f });
			uiObject->SetAnchorMin(UIAnchor{ 0.0f, 0.0f });;
			uiObject->SetAnchorMax(UIAnchor{ 0.0f, 0.0f });;
			uiObject->SetPivot(UIAnchor{ 0.0f, 0.0f });;
			uiObject->AddComponent<UIComponent>();
			uiObject->UpdateInteractableFlags();
			uiManager->AddUI(sceneName, uiObject);
			uiManager->RefreshUIListForCurrentScene();
			m_SelectedUIObjectName = name;
			m_SelectedUIObjectNames.clear();
			m_SelectedUIObjectNames.insert(name);

			auto uiObjectRef = uiObject;
			m_UndoManager.Push(UndoManager::Command{
				"Create UI Object",
				[this, uiManager, sceneName, uiObjectRef]()
				{
					if (!uiManager || !uiObjectRef)
						return;

					uiManager->RemoveUI(sceneName, uiObjectRef);
					uiManager->RefreshUIListForCurrentScene();
					m_SelectedUIObjectNames.erase(uiObjectRef->GetName());
					if (m_SelectedUIObjectName == uiObjectRef->GetName())
					{
						m_SelectedUIObjectName.clear();
					}
				},
				[this, uiManager, sceneName, uiObjectRef]()
				{
					if (!uiManager || !uiObjectRef)
						return;

					uiManager->AddUI(sceneName, uiObjectRef);
					uiManager->RefreshUIListForCurrentScene();
					m_SelectedUIObjectName = uiObjectRef->GetName();
					m_SelectedUIObjectNames.clear();
					m_SelectedUIObjectNames.insert(uiObjectRef->GetName());
				}
				});
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove Selected"))
		{
			auto selectedObject = getUIObjectByName(m_SelectedUIObjectName);
			if (selectedObject)
			{
				auto removedObject = selectedObject;
				uiManager->RemoveUI(sceneName, selectedObject);
				uiManager->RefreshUIListForCurrentScene();
				m_SelectedUIObjectName.clear();
				m_SelectedUIObjectNames.erase(removedObject->GetName());

				m_UndoManager.Push(UndoManager::Command{
					"Remove UI Object",
					[this, uiManager, sceneName, removedObject]()
					{
						if (!uiManager || !removedObject)
							return;

						uiManager->AddUI(sceneName, removedObject);
						uiManager->RefreshUIListForCurrentScene();
						m_SelectedUIObjectName = removedObject->GetName();
						m_SelectedUIObjectNames.clear();
						m_SelectedUIObjectNames.insert(removedObject->GetName());
					},
					[this, uiManager, sceneName, removedObject]()
					{
						if (!uiManager || !removedObject)
							return;

						uiManager->RemoveUI(sceneName, removedObject);
						uiManager->RefreshUIListForCurrentScene();
						m_SelectedUIObjectNames.erase(removedObject->GetName());
						if (m_SelectedUIObjectName == removedObject->GetName())
						{
							m_SelectedUIObjectName.clear();
						}
					}
					});
			}
		}
		ImGui::Separator();

		if (it == uiObjectsByScene.end() || it->second.empty())
		{
			ImGui::TextDisabled("No UI Objects in Scene.");
		}
		else
		{
			auto buildChildren = [&](const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
				{
					std::unordered_map<std::string, std::vector<std::string>> children;
					std::vector<std::string> roots;

					for (const auto& [name, uiObject] : uiMap)
					{
						if (!uiObject)
						{
							continue;
						}

						const std::string& parentName = uiObject->GetParentName();
						if (parentName.empty() || uiMap.find(parentName) == uiMap.end())
						{
							roots.push_back(name);
						}
						else
						{
							children[parentName].push_back(name);
						}
					}

					for (auto& [parentName, childList] : children)
					{
						std::sort(childList.begin(), childList.end());
					}
					std::sort(roots.begin(), roots.end());

					return std::make_pair(roots, children);
				};

			auto isDescendant = [&](const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
				const std::string& candidate, const std::string& possibleAncestor) -> bool
				{
					std::unordered_set<std::string> visited;
					std::string current = candidate;
					while (!current.empty())
					{
						if (current == possibleAncestor)
						{
							return true;
						}
						if (!visited.insert(current).second)
						{
							break;
						}
						auto itNode = uiMap.find(current);
						if (itNode == uiMap.end() || !itNode->second)
						{
							break;
						}
						current = itNode->second->GetParentName();
					}
					return false;
				};

			auto [roots, children] = buildChildren(it->second);
			const std::string payloadType = "UIOBJECT_NAME";

			std::function<void(const std::string&)> drawNode = [&](const std::string& name)
				{
					auto itNode = it->second.find(name);
					if (itNode == it->second.end() || !itNode->second)
					{
						return;
					}

					const bool selected = (m_SelectedUIObjectNames.find(name) != m_SelectedUIObjectNames.end());
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
					if (children.find(name) == children.end())
					{
						flags |= ImGuiTreeNodeFlags_Leaf;
					}
					if (selected)
					{
						flags |= ImGuiTreeNodeFlags_Selected;
					}

					bool opened = ImGui::TreeNodeEx(name.c_str(), flags);
					if (ImGui::IsItemClicked())
					{
						const bool append = ImGui::GetIO().KeyShift;
						if (!append)
						{
							m_SelectedUIObjectNames.clear();
						}
						if (!m_SelectedUIObjectNames.insert(name).second && append)
						{
							m_SelectedUIObjectNames.erase(name);
						}
						m_SelectedUIObjectName = name;
					}

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload(payloadType.c_str(), name.c_str(), name.size() + 1);
						ImGui::Text("Move: %s", name.c_str());
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
						{
							const char* droppedName = static_cast<const char*>(payload->Data);
							if (droppedName && name != droppedName)
							{
								auto itDrop = it->second.find(droppedName);
								if (itDrop != it->second.end() && itDrop->second)
								{
									if (!isDescendant(it->second, name, droppedName))
									{
										nlohmann::json beforeSnapshot;
										itDrop->second->Serialize(beforeSnapshot);
										itDrop->second->SetParentName(name);
										nlohmann::json afterSnapshot;
										itDrop->second->Serialize(afterSnapshot);
										pushUISnapshotUndo("Change UI Parent", beforeSnapshot, afterSnapshot);
									}
								}
							}
						}
						ImGui::EndDragDropTarget();
					}

					if (opened)
					{
						auto childIt = children.find(name);
						if (childIt != children.end())
						{
							for (const auto& childName : childIt->second)
							{
								drawNode(childName);
							}
						}
						ImGui::TreePop();
					}
				};

			ImGui::SeparatorText("Hierarchy");
			if (!roots.empty())
			{
				for (const auto& root : roots)
				{
					drawNode(root);
				}
			}
			else
			{
				for (const auto& [name, uiObject] : it->second)
				{
					if (uiObject)
					{
						drawNode(name);
					}
				}
			}
		}

		auto isUIComponentType = [](const std::string& typeName) -> bool
			{
				auto& registry = ComponentRegistry::Instance();
				auto* typeInfo = registry.Find(typeName);
				while (typeInfo)
				{
					if (typeInfo->name == UIComponent::StaticTypeName)
					{
						return true;
					}
					if (!typeInfo->parent && typeInfo->parentName)
					{
						typeInfo->parent = registry.Find(typeInfo->parentName);
					}
					typeInfo = typeInfo->parent;
				}
				return false;
			};

		auto selectedObject = getUIObjectByName(m_SelectedUIObjectName);
		if (selectedObject)
		{
			if (m_LastSelectedUIObjectName != selectedObject->GetName())
			{
				m_PendingUIPropertySnapshots.clear();
				m_LastSelectedUIObjectName = selectedObject->GetName();
			}

			auto recordUILongEdit = [&](size_t key, bool updated, const std::string& label)
				{
					if (ImGui::IsItemActivated() && m_PendingUIPropertySnapshots.find(key) == m_PendingUIPropertySnapshots.end())
					{
						PendingUIPropertySnapshot pendingSnapshot;
						selectedObject->Serialize(pendingSnapshot.beforeSnapshot);
						m_PendingUIPropertySnapshots.emplace(key, std::move(pendingSnapshot));
					}

					if (updated)
					{
						auto itSnapshot = m_PendingUIPropertySnapshots.find(key);
						if (itSnapshot != m_PendingUIPropertySnapshots.end())
						{
							itSnapshot->second.updated = true;
						}
					}

					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						auto itSnapshot = m_PendingUIPropertySnapshots.find(key);
						if (itSnapshot != m_PendingUIPropertySnapshots.end())
						{
							if (itSnapshot->second.updated)
							{
								nlohmann::json afterSnapshot;
								selectedObject->Serialize(afterSnapshot);
								pushUISnapshotUndo(label, itSnapshot->second.beforeSnapshot, afterSnapshot);
							}
							m_PendingUIPropertySnapshots.erase(itSnapshot);
						}
					}
				};

			auto makeUILayoutKey = [&](const std::string& propertyName)
				{
					const size_t pointerHash = std::hash<const void*>{}(selectedObject.get());
					const size_t nameHash = std::hash<std::string>{}(propertyName);
					return pointerHash ^ (nameHash + 0x9e3779b97f4a7c15ULL + (pointerHash << 6) + (pointerHash >> 2));
				};

			ImGui::SeparatorText("Layout");
			ImGui::Text("Parent");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##ParentName", selectedObject->GetParentName().empty() ? "<None>" : selectedObject->GetParentName().c_str()))
			{
				if (ImGui::Selectable("<None>", selectedObject->GetParentName().empty()))
				{
					nlohmann::json beforeSnapshot;
					selectedObject->Serialize(beforeSnapshot);
					selectedObject->ClearParentName();
					nlohmann::json afterSnapshot;
					selectedObject->Serialize(afterSnapshot);
					pushUISnapshotUndo("Change UI Parent", beforeSnapshot, afterSnapshot);
				}
				for (const auto& [name, uiObject] : it->second)
				{
					if (!uiObject || name == selectedObject->GetName())
					{
						continue;
					}
					const bool isSelected = (selectedObject->GetParentName() == name);
					if (ImGui::Selectable(name.c_str(), isSelected))
					{
						nlohmann::json beforeSnapshot;
						selectedObject->Serialize(beforeSnapshot);
						selectedObject->SetParentName(name);
						nlohmann::json afterSnapshot;
						selectedObject->Serialize(afterSnapshot);
						pushUISnapshotUndo("Change UI Parent", beforeSnapshot, afterSnapshot);
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			auto clampAnchor = [](UIAnchor anchor)
				{
					anchor.x = std::clamp(anchor.x, 0.0f, 1.0f);
					anchor.y = std::clamp(anchor.y, 0.0f, 1.0f);
					return anchor;
				};

			UIAnchor anchorMin = selectedObject->GetAnchorMin();
			UIAnchor anchorMax = selectedObject->GetAnchorMax();
			UIAnchor pivot = selectedObject->GetPivot();
			float rotation = selectedObject->GetRotationDegrees();
			float anchorMinValues[2] = { anchorMin.x, anchorMin.y };
			float anchorMaxValues[2] = { anchorMax.x, anchorMax.y };
			float pivotValues[2] = { pivot.x, pivot.y };

			const bool anchorMinChanged = ImGui::DragFloat2("Anchor Min", anchorMinValues, 0.01f, 0.0f, 1.0f);
			if (anchorMinChanged)
			{
				selectedObject->SetAnchorMin(clampAnchor(UIAnchor{ anchorMinValues[0], anchorMinValues[1] }));
			}
			recordUILongEdit(makeUILayoutKey("AnchorMin"), anchorMinChanged, "Edit UI Anchor Min");

			const bool anchorMaxChanged = ImGui::DragFloat2("Anchor Max", anchorMaxValues, 0.01f, 0.0f, 1.0f);
			if (anchorMaxChanged)
			{
				selectedObject->SetAnchorMax(clampAnchor(UIAnchor{ anchorMaxValues[0], anchorMaxValues[1] }));
			}
			recordUILongEdit(makeUILayoutKey("AnchorMax"), anchorMaxChanged, "Edit UI Anchor Max");

			const bool pivotChanged = ImGui::DragFloat2("Pivot", pivotValues, 0.01f, 0.0f, 1.0f);
			if (pivotChanged)
			{
				selectedObject->SetPivot(clampAnchor(UIAnchor{ pivotValues[0], pivotValues[1] }));
			}
			recordUILongEdit(makeUILayoutKey("Pivot"), pivotChanged, "Edit UI Pivot");

			const bool rotationChanged = ImGui::DragFloat("Rotation", &rotation, 0.5f, -180.0f, 180.0f);
			if (rotationChanged)
			{
				selectedObject->SetRotationDegrees(rotation);
			}
			recordUILongEdit(makeUILayoutKey("Rotation"), rotationChanged, "Edit UI Rotation");

			ImGui::SeparatorText("Align");
			if (m_SelectedUIObjectNames.size() > 1)
			{
				auto reference = getUIObjectByName(m_SelectedUIObjectName);
				if (reference)
				{
					const UIRect referenceBounds = reference->GetBounds();
					auto applyAlignment = [&](const std::function<void(UIRect&)>& action)
						{
							for (const auto& name : m_SelectedUIObjectNames)
							{
								auto target = getUIObjectByName(name);
								if (!target)
								{
									continue;
								}
								UIRect bounds = target->GetBounds();
								action(bounds);
								target->SetBounds(bounds);
							}
						};

					if (ImGui::Button("Left"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.x = referenceBounds.x; });
					}
					ImGui::SameLine();
					if (ImGui::Button("Right"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.x = referenceBounds.x + referenceBounds.width - bounds.width; });
					}
					ImGui::SameLine();
					if (ImGui::Button("H Center"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.x = referenceBounds.x + (referenceBounds.width - bounds.width) * 0.5f; });
					}

					if (ImGui::Button("Top"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.y = referenceBounds.y; });
					}
					ImGui::SameLine();
					if (ImGui::Button("Bottom"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.y = referenceBounds.y + referenceBounds.height - bounds.height; });
					}
					ImGui::SameLine();
					if (ImGui::Button("V Center"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.y = referenceBounds.y + (referenceBounds.height - bounds.height) * 0.5f; });
					}
				}
			}
			else
			{
				ImGui::TextDisabled("Select multiple UI objects to align.");
			}

			ImGui::SeparatorText("Components");

			auto componentTypes = selectedObject->GetComponentTypeNames();
			for (const auto& componentType : componentTypes)
			{
				if (!isUIComponentType(componentType))
				{
					continue;
				}

				ImGui::PushID(componentType.c_str());
				ImGui::Text("%s", componentType.c_str());
				ImGui::SameLine();
				if (ImGui::Button("Remove"))
				{
					nlohmann::json beforeSnapshot;
					selectedObject->Serialize(beforeSnapshot);
					selectedObject->RemoveComponentByTypeName(componentType);
					selectedObject->UpdateInteractableFlags();
					nlohmann::json afterSnapshot;
					selectedObject->Serialize(afterSnapshot);
					pushUISnapshotUndo("Remove UI Component", beforeSnapshot, afterSnapshot);
				}
				ImGui::PopID();
			}

			std::vector<std::string> uiComponentTypes;
			for (const auto& name : ComponentRegistry::Instance().GetTypeNames())
			{
				if (isUIComponentType(name))
				{
					uiComponentTypes.push_back(name);
				}
			}

			if (!uiComponentTypes.empty())
			{
				static int selectedTypeIndex = 0;
				selectedTypeIndex = std::clamp(selectedTypeIndex, 0, static_cast<int>(uiComponentTypes.size() - 1));
				if (ImGui::BeginCombo("Add UI Component", uiComponentTypes[selectedTypeIndex].c_str()))
				{
					for (int i = 0; i < static_cast<int>(uiComponentTypes.size()); ++i)
					{
						const bool isSelected = (selectedTypeIndex == i);
						if (ImGui::Selectable(uiComponentTypes[i].c_str(), isSelected))
						{
							selectedTypeIndex = i;
						}
						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				const std::string& selectedTypeName = uiComponentTypes[selectedTypeIndex];
				const bool alreadyHasComponent = !selectedObject->GetComponentsByTypeName(selectedTypeName).empty();
				ImGui::BeginDisabled(alreadyHasComponent);
				if (ImGui::Button("Add Component"))
				{
					nlohmann::json beforeSnapshot;
					selectedObject->Serialize(beforeSnapshot);
					selectedObject->AddComponentByTypeName(selectedTypeName);
					selectedObject->UpdateInteractableFlags();
					nlohmann::json afterSnapshot;
					selectedObject->Serialize(afterSnapshot);
					pushUISnapshotUndo("Add UI Component", beforeSnapshot, afterSnapshot);
				}
				ImGui::EndDisabled();
				if (alreadyHasComponent)
				{
					ImGui::SameLine();
					ImGui::TextDisabled("Already added");
				}
			}
			else
			{
				ImGui::TextDisabled("No UI component types registered.");
			}

			if (m_AssetLoader)
			{
				ImGui::SeparatorText("Properties");
				auto componentTypesForEdit = selectedObject->GetComponentTypeNames();
				for (const auto& componentType : componentTypesForEdit)
				{
					if (!isUIComponentType(componentType))
					{
						continue;
					}

					ImGui::PushID(componentType.c_str());
					const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
					if (ImGui::TreeNodeEx(componentType.c_str(), flags))
					{
						Component* component = selectedObject->GetComponentByTypeName(componentType);
						auto* typeInfo = ComponentRegistry::Instance().Find(componentType);
						if (component && typeInfo)
						{
							auto props = ComponentRegistry::Instance().CollectProperties(typeInfo);
							for (const auto& prop : props)
							{
								const PropertyEditResult editResult = DrawComponentPropertyEditor(component, *prop, *m_AssetLoader);
								const bool updated = editResult.updated;
								const bool activated = editResult.activated;
								const bool deactivated = editResult.deactivated;
								const size_t propertyKey = MakePropertyKey(component, prop->GetName());

								if (activated && m_PendingUIPropertySnapshots.find(propertyKey) == m_PendingUIPropertySnapshots.end())
								{
									PendingUIPropertySnapshot pendingSnapshot;
									selectedObject->Serialize(pendingSnapshot.beforeSnapshot);
									m_PendingUIPropertySnapshots.emplace(propertyKey, std::move(pendingSnapshot));
								}

								if (updated)
								{
									auto itSnapshot = m_PendingUIPropertySnapshots.find(propertyKey);
									if (itSnapshot != m_PendingUIPropertySnapshots.end())
									{
										itSnapshot->second.updated = true;
									}
								}

								if (deactivated)
								{
									auto itSnapshot = m_PendingUIPropertySnapshots.find(propertyKey);
									if (itSnapshot != m_PendingUIPropertySnapshots.end())
									{
										if (itSnapshot->second.updated)
										{
											nlohmann::json afterSnapshot;
											selectedObject->Serialize(afterSnapshot);
											const std::string label = "Edit UI " + prop->GetName();
											pushUISnapshotUndo(label, itSnapshot->second.beforeSnapshot, afterSnapshot);
										}
										m_PendingUIPropertySnapshots.erase(itSnapshot);
									}
								}
							}
						}
						else
						{
							ImGui::TextDisabled("Component data unavailable.");
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
			}
			else
			{
				ImGui::TextDisabled("Asset loader not available for property editing.");
			}
		}
	
		else
		{
			m_PendingUIPropertySnapshots.clear();
			m_LastSelectedUIObjectName.clear();
		}
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::BeginChild("CanvasPanel", ImVec2(1920, 1080), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::Text("Canvas Preview");
		ImGui::Separator();
		const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		const ImVec2 canvasAvail = ImGui::GetContentRegionAvail();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		static bool isDragging = false;
		static std::string draggingName;
		static ImVec2 dragOffset{ 0.0f, 0.0f };
		static bool snapEnabled = true;
		static float snapSize = 10.0f;
		static std::unordered_map<std::string, UIRect> dragStartWorldBounds;
		static std::unordered_map<std::string, nlohmann::json> dragStartSnapshots;
		static std::unordered_map<std::string, nlohmann::json> dragEndSnapshots;
		static std::unordered_map<std::string, float> dragStartRotations;
		static UIRect dragStartSelectionBounds{};
		static bool hasDragSelectionBounds = false;
		static ImVec2 dragRotationCenter{ 0.0f, 0.0f };
		static float dragStartAngle = 0.0f;
		enum class HandleDragMode
		{
			None,
			Move,
			ResizeTL,
			ResizeTR,
			ResizeBL,
			ResizeBR,
			Rotate
		};
		static HandleDragMode dragMode = HandleDragMode::None;

		ImGui::Checkbox("Snap", &snapEnabled);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat("##SnapSize", &snapSize, 1.0f, 1.0f, 200.0f, "%.0f");

		if (drawList && scene && uiManager)
		{
			const auto& uiObjectsByScene = uiManager->GetUIObjects();
			const auto sceneName = scene->GetName();
			const auto it = uiObjectsByScene.find(sceneName);

			auto getWorldBounds = [&](const std::string& name,
				const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
				auto&& getWorldBoundsRef,
				std::unordered_map<std::string, UIRect>& cache,
				std::unordered_set<std::string>& visiting) -> UIRect
				{
					auto cached = cache.find(name);
					if (cached != cache.end())
					{
						return cached->second;
					}

					auto itObj = uiMap.find(name);
					if (itObj == uiMap.end() || !itObj->second)
					{
						return UIRect{};
					}

					if (!visiting.insert(name).second)
					{
						return itObj->second->GetBounds();
					}

					const auto& uiObject = itObj->second;
					UIRect local = uiObject->GetBounds();
					const std::string& parentName = uiObject->GetParentName();
					if (parentName.empty() || uiMap.find(parentName) == uiMap.end())
					{
						cache[name] = local;
						visiting.erase(name);
						return local;
					}

					UIRect parentBounds = getWorldBoundsRef(parentName, uiMap, getWorldBoundsRef, cache, visiting);
					const UIAnchor anchorMin = uiObject->GetAnchorMin();
					const UIAnchor anchorMax = uiObject->GetAnchorMax();
					const UIAnchor pivot = uiObject->GetPivot();

					const float anchorLeft = parentBounds.x + parentBounds.width * anchorMin.x;
					const float anchorTop = parentBounds.y + parentBounds.height * anchorMin.y;
					const float anchorRight = parentBounds.x + parentBounds.width * anchorMax.x;
					const float anchorBottom = parentBounds.y + parentBounds.height * anchorMax.y;

					const bool stretchX = anchorMin.x != anchorMax.x;
					const bool stretchY = anchorMin.y != anchorMax.y;
					const float baseWidth = stretchX ? (anchorRight - anchorLeft) : 0.0f;
					const float baseHeight = stretchY ? (anchorBottom - anchorTop) : 0.0f;

					const float width = stretchX ? (baseWidth + local.width) : local.width;
					const float height = stretchY ? (baseHeight + local.height) : local.height;

					UIRect world;
					world.width = width;
					world.height = height;
					world.x = anchorLeft + local.x - width * pivot.x;
					world.y = anchorTop + local.y - height * pivot.y;

					cache[name] = world;
					visiting.erase(name);
					return world;
				};

			auto setLocalFromWorld = [&](UIObject& uiObject, const UIRect& worldBounds, const UIRect& parentBounds)
				{
					const UIAnchor anchorMin = uiObject.GetAnchorMin();
					const UIAnchor anchorMax = uiObject.GetAnchorMax();
					const UIAnchor pivot = uiObject.GetPivot();

					const float anchorLeft = parentBounds.x + parentBounds.width * anchorMin.x;
					const float anchorTop = parentBounds.y + parentBounds.height * anchorMin.y;
					const float anchorRight = parentBounds.x + parentBounds.width * anchorMax.x;
					const float anchorBottom = parentBounds.y + parentBounds.height * anchorMax.y;

					const bool stretchX = anchorMin.x != anchorMax.x;
					const bool stretchY = anchorMin.y != anchorMax.y;
					const float baseWidth = stretchX ? (anchorRight - anchorLeft) : 0.0f;
					const float baseHeight = stretchY ? (anchorBottom - anchorTop) : 0.0f;

					UIRect local = uiObject.GetBounds();
					local.width = stretchX ? (worldBounds.width - baseWidth) : worldBounds.width;
					local.height = stretchY ? (worldBounds.height - baseHeight) : worldBounds.height;
					local.x = worldBounds.x - anchorLeft + worldBounds.width * pivot.x;
					local.y = worldBounds.y - anchorTop + worldBounds.height * pivot.y;
					uiObject.SetBounds(local);
				};

			auto captureUISnapshots = [&](const std::unordered_set<std::string>& names,
				std::unordered_map<std::string, nlohmann::json>& outSnapshots)
				{
					outSnapshots.clear();
					if (it == uiObjectsByScene.end())
					{
						return;
					}
					for (const auto& name : names)
					{
						auto itObj = it->second.find(name);
						if (itObj != it->second.end() && itObj->second)
						{
							nlohmann::json snapshot;
							itObj->second->Serialize(snapshot);
							outSnapshots.emplace(name, std::move(snapshot));
						}
					}
				};

			auto applyUISnapshots = [&](const std::unordered_map<std::string, nlohmann::json>& snapshots)
				{
					if (!uiManager)
					{
						return;
					}
					auto& map = uiManager->GetUIObjects();
					auto itScene = map.find(sceneName);
					if (itScene == map.end())
					{
						return;
					}
					for (const auto& [name, snapshot] : snapshots)
					{
						auto itObj = itScene->second.find(name);
						if (itObj != itScene->second.end() && itObj->second)
						{
							itObj->second->DeSerialize(snapshot);
							itObj->second->UpdateInteractableFlags();
						}
					}
				};

			ImVec2 maxExtent{ 0.0f, 0.0f };
			if (it != uiObjectsByScene.end())
			{
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;

				for (const auto& [name, uiObject] : it->second)
				{
					if (uiObject && uiObject->HasBounds())
					{
						const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
						maxExtent.x = max(maxExtent.x, bounds.x + bounds.width);
						maxExtent.y = max(maxExtent.y, bounds.y + bounds.height);
					}
				}
			}

			const float scaleX = (maxExtent.x > 0.0f) ? (canvasAvail.x / maxExtent.x) : 1.0f;
			const float scaleY = (maxExtent.y > 0.0f) ? (canvasAvail.y / maxExtent.y) : 1.0f;
			const float scale = std::min(scaleX, scaleY);

			const ImU32 background = IM_COL32(30, 30, 36, 255);
			drawList->AddRectFilled(canvasPos, { canvasPos.x + canvasAvail.x, canvasPos.y + canvasAvail.y }, background, 6.0f);
			drawList->AddRect(canvasPos, { canvasPos.x + canvasAvail.x, canvasPos.y + canvasAvail.y }, IM_COL32(0, 0, 0, 180), 6.0f);

			if (snapEnabled && snapSize > 1.0f)
			{
				for (float x = canvasPos.x; x < canvasPos.x + canvasAvail.x; x += snapSize)
				{
					drawList->AddLine({ x, canvasPos.y }, { x, canvasPos.y + canvasAvail.y }, IM_COL32(45, 45, 50, 90));
				}
				for (float y = canvasPos.y; y < canvasPos.y + canvasAvail.y; y += snapSize)
				{
					drawList->AddLine({ canvasPos.x, y }, { canvasPos.x + canvasAvail.x, y }, IM_COL32(45, 45, 50, 90));
				}
			}

			UIObject* selectedObject = nullptr;
			if (it != uiObjectsByScene.end())
			{
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;

				for (const auto& [name, uiObject] : it->second)
				{
					if (!uiObject || !uiObject->HasBounds())
					{
						continue;
					}

					const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
					const ImVec2 min = { canvasPos.x + bounds.x * scale, canvasPos.y + bounds.y * scale };
					const ImVec2 max = { min.x + bounds.width * scale, min.y + bounds.height * scale };
					const bool selected = (m_SelectedUIObjectNames.find(uiObject->GetName()) != m_SelectedUIObjectNames.end());
					if (selected)
					{
						selectedObject = uiObject.get();
					}

					bool isVisible = true;
					if (auto* baseComponent = uiObject->GetComponent<UIComponent>())
					{
						isVisible = baseComponent->GetVisible();
					}
					if (!isVisible)
					{
						continue;
					}

					const ImU32 fillColor = selected ? IM_COL32(80, 130, 220, 120) : IM_COL32(90, 90, 110, 120);
					const ImU32 outlineColor = selected ? IM_COL32(120, 160, 255, 255) : IM_COL32(0, 0, 0, 160);

					const float rotationDegrees = uiObject->GetRotationDegrees();
					const float rotationRadians = XMConvertToRadians(rotationDegrees);
					const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
					const ImVec2 half = { (max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f };
					auto rotatePoint = [&](const ImVec2& point)
						{
							const float cosA = std::cos(rotationRadians);
							const float sinA = std::sin(rotationRadians);
							const ImVec2 local = { point.x - center.x, point.y - center.y };
							return ImVec2{
								center.x + local.x * cosA - local.y * sinA,
								center.y + local.x * sinA + local.y * cosA
							};
						};

					ImVec2 corners[4] = {
						rotatePoint({ center.x - half.x, center.y - half.y }),
						rotatePoint({ center.x + half.x, center.y - half.y }),
						rotatePoint({ center.x + half.x, center.y + half.y }),
						rotatePoint({ center.x - half.x, center.y + half.y })
					};

					drawList->AddConvexPolyFilled(corners, 4, fillColor);
					drawList->AddPolyline(corners, 4, outlineColor, true, 2.0f);

					const ImVec2 textPos = { min.x + 6.0f, min.y + 6.0f };
					drawList->AddText(textPos, IM_COL32(230, 230, 230, 255), uiObject->GetName().c_str());

					if (auto* textComponent = uiObject->GetComponent<UITextComponent>())
					{
						const auto& text = textComponent->GetText();
						if (!text.empty())
						{
							drawList->AddText({ min.x + 6.0f, min.y + 24.0f }, IM_COL32(240, 240, 240, 255), text.c_str());
						}
					}

					if (auto* buttonComponent = uiObject->GetComponent<UIButtonComponent>())
					{
						ImU32 buttonOutline = IM_COL32(100, 100, 100, 200);
						if (buttonComponent->GetIsHovered())
						{
							buttonOutline = IM_COL32(160, 200, 255, 255);
						}
						if (buttonComponent->GetIsPressed())
						{
							buttonOutline = IM_COL32(120, 160, 220, 255);
						}
						drawList->AddPolyline(corners, 4, buttonOutline, true, 2.0f);
					}

					if (auto* sliderComponent = uiObject->GetComponent<UISliderComponent>())
					{
						const float normalized = sliderComponent->GetNormalizedValue();
						const ImVec2 barMin = { min.x + 6.0f, max.y - 16.0f };
						const ImVec2 barMax = { max.x - 6.0f, max.y - 8.0f };
						const float barWidth = max(0.0f, barMax.x - barMin.x);
						const ImVec2 fillMax = { barMin.x + barWidth * normalized, barMax.y };
						drawList->AddRectFilled(barMin, barMax, IM_COL32(60, 60, 60, 180), 2.0f);
						drawList->AddRectFilled(barMin, fillMax, IM_COL32(120, 180, 255, 220), 2.0f);
					}

					if (auto* progressComponent = uiObject->GetComponent<UIProgressBarComponent>())
					{
						const float percent = std::clamp(progressComponent->GetPercent(), 0.0f, 1.0f);
						const ImVec2 barMin = { min.x + 6.0f, max.y - 12.0f };
						const ImVec2 barMax = { max.x - 6.0f, max.y - 6.0f };
						const float barWidth = max(0.0f, barMax.x - barMin.x);
						const ImVec2 fillMax = { barMin.x + barWidth * percent, barMax.y };
						drawList->AddRectFilled(barMin, barMax, IM_COL32(50, 50, 50, 160), 2.0f);
						drawList->AddRectFilled(barMin, fillMax, IM_COL32(140, 220, 140, 220), 2.0f);
					}
				}
			}

			if (m_SelectedUIObjectNames.size() > 1 && it != uiObjectsByScene.end())
			{
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;
				bool first = true;
				UIRect combined{};

				for (const auto& name : m_SelectedUIObjectNames)
				{
					auto itObj = it->second.find(name);
					if (itObj == it->second.end() || !itObj->second)
					{
						continue;
					}
					const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
					if (first)
					{
						combined = bounds;
						first = false;
					}
					else
					{
						const float minX = min(combined.x, bounds.x);
						const float minY = min(combined.y, bounds.y);
						const float maxX = max(combined.x + combined.width, bounds.x + bounds.width);
						const float maxY = max(combined.y + combined.height, bounds.y + bounds.height);
						combined.x = minX;
						combined.y = minY;
						combined.width = maxX - minX;
						combined.height = maxY - minY;
					}
				}

				if (!first)
				{
					const ImVec2 min = { canvasPos.x + combined.x * scale, canvasPos.y + combined.y * scale };
					const ImVec2 max = { min.x + combined.width * scale, min.y + combined.height * scale };
					drawList->AddRect(min, max, IM_COL32(160, 200, 255, 180), 0.0f, 0, 2.0f);
					const float handleSize = 6.0f;
					const ImU32 handleColor = IM_COL32(200, 200, 200, 255);
					std::array<ImVec2, 4> corners = { min, {max.x, min.y}, {max.x, max.y}, {min.x, max.y} };
					for (const auto& corner : corners)
					{
						drawList->AddRectFilled({ corner.x - handleSize, corner.y - handleSize }, { corner.x + handleSize, corner.y + handleSize }, handleColor);
					}
					const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
					const ImVec2 topCenter = { (min.x + max.x) * 0.5f, min.y };
					ImVec2 direction = { topCenter.x - center.x, topCenter.y - center.y };
					const float dirLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
					if (dirLength > 0.001f)
					{
						direction.x /= dirLength;
						direction.y /= dirLength;
					}
					else
					{
						direction = { 0.0f, -1.0f };
					}
					const ImVec2 rotationHandle = { topCenter.x + direction.x * 20.0f, topCenter.y + direction.y * 20.0f };
					drawList->AddLine(topCenter, rotationHandle, IM_COL32(160, 200, 255, 200), 2.0f);
					drawList->AddCircleFilled(rotationHandle, 5.0f, IM_COL32(200, 200, 200, 255));
				}
			}

			ImGui::InvisibleButton("CanvasHitBox", canvasAvail);
			const bool canvasHovered = ImGui::IsItemHovered();
			ImGuiIO& io = ImGui::GetIO();

			auto getHandleHit = [&](const ImVec2& mouse,
				const std::array<ImVec2, 4>& corners,
				const ImVec2& rotationHandle) -> HandleDragMode
				{
					const float handleSize = 8.0f;
					const float rotationRadius = 8.0f;

					auto hit = [&](const ImVec2& center)
						{
							return mouse.x >= center.x - handleSize && mouse.x <= center.x + handleSize
								&& mouse.y >= center.y - handleSize && mouse.y <= center.y + handleSize;
						};

					auto hitCircle = [&](const ImVec2& center)
						{
							const float dx = mouse.x - center.x;
							const float dy = mouse.y - center.y;
							return (dx * dx + dy * dy) <= rotationRadius * rotationRadius;
						};

					if (hit(corners[0])) return HandleDragMode::ResizeTL;
					if (hit(corners[1])) return HandleDragMode::ResizeTR;
					if (hit(corners[3])) return HandleDragMode::ResizeBL;
					if (hit(corners[2])) return HandleDragMode::ResizeBR;
					if (hitCircle(rotationHandle)) return HandleDragMode::Rotate;
					return HandleDragMode::None;
				};

			if (selectedObject && selectedObject->HasBounds() && m_SelectedUIObjectNames.size() == 1)
			{
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;
				const auto bounds = getWorldBounds(selectedObject->GetName(), it->second, getWorldBounds, boundsCache, visiting);
				const ImVec2 min = { canvasPos.x + bounds.x * scale, canvasPos.y + bounds.y * scale };
				const ImVec2 max = { min.x + bounds.width * scale, min.y + bounds.height * scale };
				const float handleSize = 6.0f;
				const ImU32 handleColor = IM_COL32(200, 200, 200, 255);

				const float rotationDegrees = selectedObject->GetRotationDegrees();
				const float rotationRadians = XMConvertToRadians(rotationDegrees);
				const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
				const ImVec2 half = { (max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f };
				auto rotatePoint = [&](const ImVec2& point)
					{
						const float cosA = std::cos(rotationRadians);
						const float sinA = std::sin(rotationRadians);
						const ImVec2 local = { point.x - center.x, point.y - center.y };
						return ImVec2{
							center.x + local.x * cosA - local.y * sinA,
							center.y + local.x * sinA + local.y * cosA
						};
					};

				ImVec2 corners[4] = {
					rotatePoint({ center.x - half.x, center.y - half.y }),
					rotatePoint({ center.x + half.x, center.y - half.y }),
					rotatePoint({ center.x + half.x, center.y + half.y }),
					rotatePoint({ center.x - half.x, center.y + half.y })
				};

				drawList->AddPolyline(corners, 4, IM_COL32(120, 160, 255, 255), true, 2.0f);
				drawList->AddRectFilled({ corners[0].x - handleSize, corners[0].y - handleSize }, { corners[0].x + handleSize, corners[0].y + handleSize }, handleColor);
				drawList->AddRectFilled({ corners[1].x - handleSize, corners[1].y - handleSize }, { corners[1].x + handleSize, corners[1].y + handleSize }, handleColor);
				drawList->AddRectFilled({ corners[2].x - handleSize, corners[2].y - handleSize }, { corners[2].x + handleSize, corners[2].y + handleSize }, handleColor);
				drawList->AddRectFilled({ corners[3].x - handleSize, corners[3].y - handleSize }, { corners[3].x + handleSize, corners[3].y + handleSize }, handleColor);
				const ImVec2 topCenter = { (corners[0].x + corners[1].x) * 0.5f, (corners[0].y + corners[1].y) * 0.5f };
				ImVec2 direction = { topCenter.x - center.x, topCenter.y - center.y };
				const float dirLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
				if (dirLength > 0.001f)
				{
					direction.x /= dirLength;
					direction.y /= dirLength;
				}
				else
				{
					direction = { 0.0f, -1.0f };
				}
				const ImVec2 rotationHandle = { topCenter.x + direction.x * 20.0f, topCenter.y + direction.y * 20.0f };
				drawList->AddLine(topCenter, rotationHandle, IM_COL32(160, 200, 255, 200), 2.0f);
				drawList->AddCircleFilled(rotationHandle, 5.0f, IM_COL32(200, 200, 200, 255));
			}

			if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && it != uiObjectsByScene.end())
			{
				const ImVec2 mousePos = io.MousePos;
				const ImVec2 localPos = { (mousePos.x - canvasPos.x) / scale, (mousePos.y - canvasPos.y) / scale };
				UIObject* hitObject = nullptr;
				int hitZ = std::numeric_limits<int>::min();
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;

				for (const auto& [name, uiObject] : it->second)
				{
					if (!uiObject || !uiObject->HasBounds())
					{
						continue;
					}

					const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
					const float rotationRadians = XMConvertToRadians(uiObject->GetRotationDegrees());
					const ImVec2 center = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
					const float cosA = std::cos(-rotationRadians);
					const float sinA = std::sin(-rotationRadians);
					const ImVec2 localPoint = {
						center.x + (localPos.x - center.x) * cosA - (localPos.y - center.y) * sinA,
						center.y + (localPos.x - center.x) * sinA + (localPos.y - center.y) * cosA
					};
					const bool inside = localPoint.x >= bounds.x && localPoint.x <= bounds.x + bounds.width
						&& localPoint.y >= bounds.y && localPoint.y <= bounds.y + bounds.height;

					if (inside && uiObject->GetZOrder() >= hitZ)
					{
						hitObject = uiObject.get();
						hitZ = uiObject->GetZOrder();
					}
				}

				if (selectedObject && selectedObject->HasBounds())
				{
					if (m_SelectedUIObjectNames.size() == 1)
					{
						const auto bounds = getWorldBounds(selectedObject->GetName(), it->second, getWorldBounds, boundsCache, visiting);
						const ImVec2 min = { canvasPos.x + bounds.x * scale, canvasPos.y + bounds.y * scale };
						const ImVec2 max = { min.x + bounds.width * scale, min.y + bounds.height * scale };
						const float rotationRadians = XMConvertToRadians(selectedObject->GetRotationDegrees());
						const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
						const ImVec2 half = { (max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f };
						auto rotatePoint = [&](const ImVec2& point)
							{
								const float cosA = std::cos(rotationRadians);
								const float sinA = std::sin(rotationRadians);
								const ImVec2 local = { point.x - center.x, point.y - center.y };
								return ImVec2{
									center.x + local.x * cosA - local.y * sinA,
									center.y + local.x * sinA + local.y * cosA
								};
							};
						std::array<ImVec2, 4> corners = {
							rotatePoint({ center.x - half.x, center.y - half.y }),
							rotatePoint({ center.x + half.x, center.y - half.y }),
							rotatePoint({ center.x + half.x, center.y + half.y }),
							rotatePoint({ center.x - half.x, center.y + half.y })
						};
						const ImVec2 topCenter = { (corners[0].x + corners[1].x) * 0.5f, (corners[0].y + corners[1].y) * 0.5f };
						ImVec2 direction = { topCenter.x - center.x, topCenter.y - center.y };
						const float dirLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
						if (dirLength > 0.001f)
						{
							direction.x /= dirLength;
							direction.y /= dirLength;
						}
						else
						{
							direction = { 0.0f, -1.0f };
						}
						const ImVec2 rotationHandle = { topCenter.x + direction.x * 20.0f, topCenter.y + direction.y * 20.0f };
						const HandleDragMode handleHit = getHandleHit(mousePos, corners, rotationHandle);
						if (handleHit != HandleDragMode::None)
						{
							m_SelectedUIObjectName = selectedObject->GetName();
							m_SelectedUIObjectNames.clear();
							m_SelectedUIObjectNames.insert(selectedObject->GetName());
							draggingName = selectedObject->GetName();
							isDragging = true;
							dragMode = handleHit;
							dragOffset = localPos;
							dragStartWorldBounds.clear();
							dragStartRotations.clear();
							dragStartWorldBounds[selectedObject->GetName()] = bounds;
							dragStartRotations[selectedObject->GetName()] = selectedObject->GetRotationDegrees();
							dragStartSelectionBounds = bounds;
							hasDragSelectionBounds = true;
							captureUISnapshots(m_SelectedUIObjectNames, dragStartSnapshots);
							if (dragMode == HandleDragMode::Rotate)
							{
								dragRotationCenter = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
								dragStartAngle = std::atan2(localPos.y - dragRotationCenter.y, localPos.x - dragRotationCenter.x);
							}
							hitObject = nullptr;
						}
					}
					else if (m_SelectedUIObjectNames.size() > 1)
					{
						UIRect combined{};
						bool first = true;
						for (const auto& name : m_SelectedUIObjectNames)
						{
							const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
							if (first)
							{
								combined = bounds;
								first = false;
							}
							else
							{
								const float minX = min(combined.x, bounds.x);
								const float minY = min(combined.y, bounds.y);
								const float maxX = max(combined.x + combined.width, bounds.x + bounds.width);
								const float maxY = max(combined.y + combined.height, bounds.y + bounds.height);
								combined.x = minX;
								combined.y = minY;
								combined.width = maxX - minX;
								combined.height = maxY - minY;
							}
						}
						if (!first)
						{
							const ImVec2 min = { canvasPos.x + combined.x * scale, canvasPos.y + combined.y * scale };
							const ImVec2 max = { min.x + combined.width * scale, min.y + combined.height * scale };
							std::array<ImVec2, 4> corners = { min, {max.x, min.y}, {max.x, max.y}, {min.x, max.y} };
							const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
							const ImVec2 topCenter = { (min.x + max.x) * 0.5f, min.y };
							ImVec2 direction = { topCenter.x - center.x, topCenter.y - center.y };
							const float dirLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
							if (dirLength > 0.001f)
							{
								direction.x /= dirLength;
								direction.y /= dirLength;
							}
							else
							{
								direction = { 0.0f, -1.0f };
							}
							const ImVec2 rotationHandle = { topCenter.x + direction.x * 20.0f, topCenter.y + direction.y * 20.0f };
							const HandleDragMode handleHit = getHandleHit(mousePos, corners, rotationHandle);
							if (handleHit != HandleDragMode::None)
							{
								isDragging = true;
								dragMode = handleHit;
								dragOffset = localPos;
								dragStartWorldBounds.clear();
								dragStartRotations.clear();
								for (const auto& name : m_SelectedUIObjectNames)
								{
									auto itObj = it->second.find(name);
									if (itObj != it->second.end() && itObj->second)
									{
										dragStartWorldBounds[name] = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
										dragStartRotations[name] = itObj->second->GetRotationDegrees();
									}
								}
								dragStartSelectionBounds = combined;
								hasDragSelectionBounds = true;
								captureUISnapshots(m_SelectedUIObjectNames, dragStartSnapshots);
								if (dragMode == HandleDragMode::Rotate)
								{
									dragRotationCenter = { combined.x + combined.width * 0.5f, combined.y + combined.height * 0.5f };
									dragStartAngle = std::atan2(localPos.y - dragRotationCenter.y, localPos.x - dragRotationCenter.x);
								}
								hitObject = nullptr;
							}
						}
					}
				}

				if (hitObject)
				{
					const bool append = io.KeyShift;
					if (!append)
					{
						m_SelectedUIObjectNames.clear();
					}
					if (!m_SelectedUIObjectNames.insert(hitObject->GetName()).second && append)
					{
						m_SelectedUIObjectNames.erase(hitObject->GetName());
					}
					m_SelectedUIObjectName = hitObject->GetName();
					draggingName = hitObject->GetName();
					isDragging = true;
					dragMode = HandleDragMode::Move;
					const auto bounds = getWorldBounds(hitObject->GetName(), it->second, getWorldBounds, boundsCache, visiting);
					dragOffset = { localPos.x - bounds.x, localPos.y - bounds.y };
					dragStartWorldBounds.clear();
					dragStartRotations.clear();
					dragStartSelectionBounds = UIRect{};
					hasDragSelectionBounds = false;
					for (const auto& name : m_SelectedUIObjectNames)
					{
						auto itObj = it->second.find(name);
						if (itObj != it->second.end() && itObj->second)
						{
							dragStartWorldBounds[name] = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
							dragStartRotations[name] = itObj->second->GetRotationDegrees();
						}
					}
					captureUISnapshots(m_SelectedUIObjectNames, dragStartSnapshots);
				}
			}

			if (isDragging && io.MouseDown[ImGuiMouseButton_Left] && it != uiObjectsByScene.end())
			{
				const ImVec2 mousePos = io.MousePos;
				const ImVec2 localPos = { (mousePos.x - canvasPos.x) / scale, (mousePos.y - canvasPos.y) / scale };
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;
				const float minSize = 10.0f;

				if (dragMode == HandleDragMode::Move)
				{
					for (const auto& name : m_SelectedUIObjectNames)
					{
						auto itObj = it->second.find(name);
						if (itObj == it->second.end() || !itObj->second)
						{
							continue;
						}

						UIRect bounds = dragStartWorldBounds.count(name)
							? dragStartWorldBounds.at(name)
							: getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
						float deltaX = localPos.x - (dragOffset.x + bounds.x);
						float deltaY = localPos.y - (dragOffset.y + bounds.y);
						if (snapEnabled && snapSize > 1.0f)
						{
							deltaX = std::round(deltaX / snapSize) * snapSize;
							deltaY = std::round(deltaY / snapSize) * snapSize;
						}
						bounds.x += deltaX;
						bounds.y += deltaY;

						UIRect parentBounds{};
						const std::string parentName = itObj->second->GetParentName();
						if (!parentName.empty() && it->second.find(parentName) != it->second.end())
						{
							parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
						}
						setLocalFromWorld(*itObj->second, bounds, parentBounds);
					}
				}
				else if (dragMode == HandleDragMode::Rotate)
				{
					const float currentAngle = std::atan2(localPos.y - dragRotationCenter.y, localPos.x - dragRotationCenter.x);
					const float deltaRadians = currentAngle - dragStartAngle;
					const float deltaDegrees = XMConvertToDegrees(deltaRadians);

					for (const auto& name : m_SelectedUIObjectNames)
					{
						auto itObj = it->second.find(name);
						if (itObj == it->second.end() || !itObj->second)
						{
							continue;
						}

						const float startRotation = dragStartRotations.count(name) ? dragStartRotations.at(name) : itObj->second->GetRotationDegrees();
						itObj->second->SetRotationDegrees(startRotation + deltaDegrees);

						if (m_SelectedUIObjectNames.size() > 1)
						{
							UIRect startBounds = dragStartWorldBounds.count(name)
								? dragStartWorldBounds.at(name)
								: getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
							const ImVec2 startCenter = { startBounds.x + startBounds.width * 0.5f, startBounds.y + startBounds.height * 0.5f };
							const float cosA = std::cos(deltaRadians);
							const float sinA = std::sin(deltaRadians);
							const ImVec2 offset = { startCenter.x - dragRotationCenter.x, startCenter.y - dragRotationCenter.y };
							const ImVec2 rotated = {
								dragRotationCenter.x + offset.x * cosA - offset.y * sinA,
								dragRotationCenter.y + offset.x * sinA + offset.y * cosA
							};
							UIRect bounds = startBounds;
							bounds.x = rotated.x - bounds.width * 0.5f;
							bounds.y = rotated.y - bounds.height * 0.5f;

							UIRect parentBounds{};
							const std::string parentName = itObj->second->GetParentName();
							if (!parentName.empty() && it->second.find(parentName) != it->second.end())
							{
								parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
							}
							setLocalFromWorld(*itObj->second, bounds, parentBounds);
						}
					}
				}
				else
				{
					if (m_SelectedUIObjectNames.size() > 1 && hasDragSelectionBounds)
					{
						UIRect selection = dragStartSelectionBounds;
						UIRect newSelection = selection;
						switch (dragMode)
						{
						case HandleDragMode::ResizeTL:
							newSelection.x = min(localPos.x, selection.x + selection.width - minSize);
							newSelection.y = min(localPos.y, selection.y + selection.height - minSize);
							newSelection.width = selection.x + selection.width - newSelection.x;
							newSelection.height = selection.y + selection.height - newSelection.y;
							break;
						case HandleDragMode::ResizeTR:
							newSelection.width = max(minSize, localPos.x - selection.x);
							newSelection.y = min(localPos.y, selection.y + selection.height - minSize);
							newSelection.height = selection.y + selection.height - newSelection.y;
							break;
						case HandleDragMode::ResizeBL:
							newSelection.x = min(localPos.x, selection.x + selection.width - minSize);
							newSelection.width = selection.x + selection.width - newSelection.x;
							newSelection.height = max(minSize, localPos.y - selection.y);
							break;
						case HandleDragMode::ResizeBR:
							newSelection.width = max(minSize, localPos.x - selection.x);
							newSelection.height = max(minSize, localPos.y - selection.y);
							break;
						default:
							break;
						}

						if (snapEnabled && snapSize > 1.0f)
						{
							newSelection.x = std::round(newSelection.x / snapSize) * snapSize;
							newSelection.y = std::round(newSelection.y / snapSize) * snapSize;
							newSelection.width = std::round(newSelection.width / snapSize) * snapSize;
							newSelection.height = std::round(newSelection.height / snapSize) * snapSize;
						}

						const float scaleX = selection.width > 0.0f ? (newSelection.width / selection.width) : 1.0f;
						const float scaleY = selection.height > 0.0f ? (newSelection.height / selection.height) : 1.0f;

						for (const auto& name : m_SelectedUIObjectNames)
						{
							auto itObj = it->second.find(name);
							if (itObj == it->second.end() || !itObj->second)
							{
								continue;
							}

							UIRect startBounds = dragStartWorldBounds.count(name)
								? dragStartWorldBounds.at(name)
								: getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);

							UIRect bounds = startBounds;
							bounds.x = newSelection.x + (startBounds.x - selection.x) * scaleX;
							bounds.y = newSelection.y + (startBounds.y - selection.y) * scaleY;
							bounds.width = max(minSize, startBounds.width * scaleX);
							bounds.height = max(minSize, startBounds.height * scaleY);

							UIRect parentBounds{};
							const std::string parentName = itObj->second->GetParentName();
							if (!parentName.empty() && it->second.find(parentName) != it->second.end())
							{
								parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
							}
							setLocalFromWorld(*itObj->second, bounds, parentBounds);
						}
					}
					else
					{
						auto found = it->second.find(draggingName);
						if (found != it->second.end() && found->second)
						{
							UIRect bounds = dragStartWorldBounds.count(found->second->GetName())
								? dragStartWorldBounds.at(found->second->GetName())
								: getWorldBounds(found->second->GetName(), it->second, getWorldBounds, boundsCache, visiting);
							const float rotationRadians = XMConvertToRadians(found->second->GetRotationDegrees());
							const ImVec2 center = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
							const float cosA = std::cos(-rotationRadians);
							const float sinA = std::sin(-rotationRadians);
							const ImVec2 localPoint = {
								center.x + (localPos.x - center.x) * cosA - (localPos.y - center.y) * sinA,
								center.y + (localPos.x - center.x) * sinA + (localPos.y - center.y) * cosA
							};

							switch (dragMode)
							{
							case HandleDragMode::ResizeTL:
								bounds.width = max(minSize, bounds.width + (bounds.x - localPoint.x));
								bounds.height = max(minSize, bounds.height + (bounds.y - localPoint.y));
								bounds.x = localPoint.x;
								bounds.y = localPoint.y;
								break;
							case HandleDragMode::ResizeTR:
								bounds.width = max(minSize, localPoint.x - bounds.x);
								bounds.height = max(minSize, bounds.height + (bounds.y - localPoint.y));
								bounds.y = localPoint.y;
								break;
							case HandleDragMode::ResizeBL:
								bounds.width = max(minSize, bounds.width + (bounds.x - localPoint.x));
								bounds.x = localPoint.x;
								bounds.height = max(minSize, localPoint.y - bounds.y);
								break;
							case HandleDragMode::ResizeBR:
								bounds.width = max(minSize, localPoint.x - bounds.x);
								bounds.height = max(minSize, localPoint.y - bounds.y);
								break;
							default:
								break;
							}

							if (snapEnabled && snapSize > 1.0f)
							{
								bounds.x = std::round(bounds.x / snapSize) * snapSize;
								bounds.y = std::round(bounds.y / snapSize) * snapSize;
								bounds.width = std::round(bounds.width / snapSize) * snapSize;
								bounds.height = std::round(bounds.height / snapSize) * snapSize;
							}

							UIRect parentBounds{};
							const std::string parentName = found->second->GetParentName();
							if (!parentName.empty() && it->second.find(parentName) != it->second.end())
							{
								parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
							}
							setLocalFromWorld(*found->second, bounds, parentBounds);
						}
					}
				}
			}

			if (isDragging && !io.MouseDown[ImGuiMouseButton_Left])
			{
				isDragging = false;
				draggingName.clear();
				dragMode = HandleDragMode::None;
				dragEndSnapshots.clear();

				if (it != uiObjectsByScene.end())
				{
					captureUISnapshots(m_SelectedUIObjectNames, dragEndSnapshots);
				}

				if (!dragStartSnapshots.empty() && !dragEndSnapshots.empty())
				{
					auto before = dragStartSnapshots;
					auto after = dragEndSnapshots;
					bool changed = false;
					for (const auto& [name, snapshot] : after)
					{
						auto itBefore = before.find(name);
						if (itBefore == before.end() || itBefore->second != snapshot)
						{
							changed = true;
							break;
						}
					}

					if (changed)
					{
						m_UndoManager.Push(UndoManager::Command{
							"Transform UI Objects",
							[this, uiManager, sceneName, before]()
							{
								if (!uiManager)
								{
									return;
								}
								auto& map = uiManager->GetUIObjects();
								auto itScene = map.find(sceneName);
								if (itScene == map.end())
								{
									return;
								}
								for (const auto& [name, snapshot] : before)
								{
									auto itObj = itScene->second.find(name);
									if (itObj != itScene->second.end() && itObj->second)
									{
										itObj->second->DeSerialize(snapshot);
										itObj->second->UpdateInteractableFlags();
									}
								}
							},
							[this, uiManager, sceneName, after]()
							{
								if (!uiManager)
								{
									return;
								}
								auto& map = uiManager->GetUIObjects();
								auto itScene = map.find(sceneName);
								if (itScene == map.end())
								{
									return;
								}
								for (const auto& [name, snapshot] : after)
								{
									auto itObj = itScene->second.find(name);
									if (itObj != itScene->second.end() && itObj->second)
									{
										itObj->second->DeSerialize(snapshot);
										itObj->second->UpdateInteractableFlags();
									}
								}
							}
							});
					}
				}
				dragStartSnapshots.clear();
				dragEndSnapshots.clear();
				dragStartWorldBounds.clear();
				dragStartRotations.clear();
				hasDragSelectionBounds = false;
			}
		}
		else
		{
			ImGui::InvisibleButton("CanvasHitBox", canvasAvail);
		}
		ImGui::EndChild();

		ImGui::Columns(1);
		ImGui::End();
	}
}


// 카메라 절두체 그림


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
	const XMFLOAT3 up = XMFLOAT3(0.0f, 0.1f, 0.0f); // editor에서는 up 고정
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

	XMFLOAT3 newEye = { target.x,target.y + 5,target.z - 5 };
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

SceneStateSnapshot EditorApplication::CaptureSceneState(const std::shared_ptr<Scene>& scene) const
{
	SceneStateSnapshot snapshot;
	snapshot.currentPath = m_CurrentScenePath;
	snapshot.selectedPath = m_SelectedResourcePath;
	snapshot.selectedObjectName = m_SelectedObjectName;
	snapshot.lastSelectedObjectName = m_LastSelectedObjectName;
	snapshot.objectNameBuffer = m_ObjectNameBuffer;
	snapshot.lastSceneName = m_LastSceneName;
	snapshot.sceneNameBuffer = m_SceneNameBuffer;

	if (scene)
	{
		snapshot.hasScene = true;
		scene->Serialize(snapshot.data);
	}
	return snapshot;
}

void EditorApplication::RestoreSceneState(const SceneStateSnapshot& snapshot)
{
	ClearPendingPropertySnapshots();

	if (snapshot.hasScene)
	{
		m_SceneManager.LoadSceneFromJsonData(snapshot.data, snapshot.currentPath);
	}
	m_CurrentScenePath = snapshot.currentPath;
	m_SelectedResourcePath = snapshot.selectedPath;
	m_SelectedObjectName = snapshot.selectedObjectName;
	m_LastSelectedObjectName = snapshot.lastSelectedObjectName;
	m_ObjectNameBuffer = snapshot.objectNameBuffer;
	m_LastSceneName = snapshot.lastSceneName;
	m_SceneNameBuffer = snapshot.sceneNameBuffer;
}

void EditorApplication::ClearPendingPropertySnapshots()
{
	m_PendingPropertySnapshots.clear();
	m_LastPendingSnapshotScenePath.clear();
}

void EditorApplication::OnResize(int width, int height)
{
	__super::OnResize(width, height);
}

void EditorApplication::OnClose()
{
	m_SceneManager.RequestQuit();
}
