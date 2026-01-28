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
#include "BoxColliderComponent.h"
#include "RayHelper.h"
#include "json.hpp"
#include "ImGuizmo.h"
#include "MathHelper.h"
#include "Snapshot.h"
#include "GameManager.h"

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
	m_GameManager = &m_Services.Get<GameManager>();
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
			UpdateLogic();  //★
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

void EditorApplication::UpdateLogic()
{
	if (m_EditorState == EditorPlayState::Play)
	{
		m_SceneManager.ChangeScene();
	}
}

void EditorApplication::Update()
{
	float dTime = m_Engine.GetTime(); 
	if (m_InputManager)
	{
		m_InputManager->SetEnabled(m_EditorState == EditorPlayState::Play);
	}
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

namespace
{
	bool BuildMeshWorldBounds(const GameObject& object, AssetLoader* assetLoader, XMFLOAT3& outMin, XMFLOAT3& outMax)
	{
		if (!assetLoader)
		{
			return false;
		}

		auto* transform = object.GetComponent<TransformComponent>();
		if (!transform)
		{
			return false;
		}

		const auto meshComponents = object.GetComponentsDerived<MeshComponent>();
		if (meshComponents.empty())
		{
			return false;
		}

		const auto world = DirectX::XMLoadFloat4x4(&transform->GetWorldMatrix());
		const MeshComponent* meshComponent = nullptr;

		for (const auto* component : meshComponents)
		{
			if (!component)
			{
				continue;
			}

			if (!component->GetMeshHandle().IsValid())
			{
				continue;
			}

			meshComponent = component;
			break;
		}

		if (!meshComponent)
		{
			return false;
		}

		const auto* meshData = assetLoader->GetMeshes().Get(meshComponent->GetMeshHandle());
		if (!meshData)
		{
			return false;
		}

		const XMFLOAT3 localMin = meshData->boundsMin;
		const XMFLOAT3 localMax = meshData->boundsMax;
		const XMFLOAT3 corners[8] = {
			{ localMin.x, localMin.y, localMin.z },
			{ localMax.x, localMin.y, localMin.z },
			{ localMax.x, localMax.y, localMin.z },
			{ localMin.x, localMax.y, localMin.z },
			{ localMin.x, localMin.y, localMax.z },
			{ localMax.x, localMin.y, localMax.z },
			{ localMax.x, localMax.y, localMax.z },
			{ localMin.x, localMax.y, localMax.z }
		};

		XMFLOAT3 minOut{ FLT_MAX, FLT_MAX, FLT_MAX };
		XMFLOAT3 maxOut{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

		for (const auto& corner : corners)
		{
			const auto v = DirectX::XMLoadFloat3(&corner);
			const auto transformed = DirectX::XMVector3TransformCoord(v, world);
			XMFLOAT3 worldCorner{};
			DirectX::XMStoreFloat3(&worldCorner, transformed);

			minOut.x = std::min(minOut.x, worldCorner.x);
			minOut.y = std::min(minOut.y, worldCorner.y);
			minOut.z = std::min(minOut.z, worldCorner.z);

			maxOut.x = (std::max)(maxOut.x, worldCorner.x);
			maxOut.y = (std::max)(maxOut.y, worldCorner.y);
			maxOut.z = (std::max)(maxOut.z, worldCorner.z);
		}

		outMin = minOut;
		outMax = maxOut;
		return true;
	}

	bool IntersectsRayBounds(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir, const XMFLOAT3& boundsMin, const XMFLOAT3& boundsMax, float& outT)
	{
		float tMin = 0.0f;
		float tMax = FLT_MAX;

		const float origin[3] = { rayOrigin.x, rayOrigin.y, rayOrigin.z };
		const float dir[3] = { rayDir.x, rayDir.y, rayDir.z };
		const float minB[3] = { boundsMin.x, boundsMin.y, boundsMin.z };
		const float maxB[3] = { boundsMax.x, boundsMax.y, boundsMax.z };

		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(dir[axis]) < 1e-6f)
			{
				if (origin[axis] < minB[axis] || origin[axis] > maxB[axis])
				{
					return false;
				}
				continue;
			}

			const float invD = 1.0f / dir[axis];
			float t0 = (minB[axis] - origin[axis]) * invD;
			float t1 = (maxB[axis] - origin[axis]) * invD;
			if (t0 > t1)
			{
				std::swap(t0, t1);
			}

			tMin = (std::max)(tMin, t0);
			tMax = (std::min)(tMax, t1);
			if (tMax < tMin)
			{
				return false;
			}
		}

		outT = tMin;
		return true;
	}
}

void EditorApplication::HandleEditorViewportSelection()
{
	if (m_EditorState != EditorPlayState::Stop)
	{
		return;
	}

	if (!m_EditorViewport.HasViewportRect() || !m_EditorViewport.IsHovered())
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		return;
	}

	if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
	{
		return;
	}

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}

	auto editorCamera = scene->GetEditorCamera().get();
	if (!editorCamera)
	{
		return;
	}

	const ImVec2 rectMin = m_EditorViewport.GetViewportRectMin();
	const ImVec2 rectMax = m_EditorViewport.GetViewportRectMax();
	const float width = rectMax.x - rectMin.x;
	const float height = rectMax.y - rectMin.y;
	if (width <= 0.0f || height <= 0.0f)
	{
		return;
	}

	const XMFLOAT4X4 viewMatrix = editorCamera->GetViewMatrix();
	const XMFLOAT4X4 projMatrix = editorCamera->GetProjMatrix();
	const auto viewMat = DirectX::XMLoadFloat4x4(&viewMatrix);
	const auto projMat = DirectX::XMLoadFloat4x4(&projMatrix);
	const Ray pickRay = MakePickRayLH(io.MousePos.x, io.MousePos.y, rectMin.x, rectMin.y, width, height, viewMat, projMat);

	const auto& gameObjects = scene->GetGameObjects();
	float closestT = FLT_MAX;
	GameObject* closestObject = nullptr;

	for (const auto& [name, object] : gameObjects)
	{
		if (!object)
		{
			continue;
		}

		XMFLOAT3 boundsMin{};
		XMFLOAT3 boundsMax{};
		bool hasBounds = false;

		if (auto* collider = object->GetComponent<BoxColliderComponent>())
		{
			if (collider->HasBounds())
			{
				hasBounds = collider->BuildWorldBounds(boundsMin, boundsMax);
			}
		}

		if (!hasBounds)
		{
			hasBounds = BuildMeshWorldBounds(*object, m_AssetLoader, boundsMin, boundsMax);
		}

		if (!hasBounds)
		{
			continue;
		}

		float hitT = 0.0f;
		if (!IntersectsRayBounds(pickRay.m_Pos, pickRay.m_Dir, boundsMin, boundsMax, hitT))
		{
			continue;
		}

		if (hitT >= 0.0f && hitT < closestT)
		{
			closestT = hitT;
			closestObject = object.get();
		}
	}

	if (!closestObject)
	{
		return;
	}

	const std::string& targetName = closestObject->GetName();
	if (targetName.empty())
	{
		return;
	}

	if (io.KeyCtrl)
	{
		if (m_SelectedObjectNames.erase(targetName) == 0)
		{
			m_SelectedObjectNames.insert(targetName);
			m_SelectedObjectName = targetName;
		}
		else if (m_SelectedObjectName == targetName)
		{
			if (!m_SelectedObjectNames.empty())
			{
				m_SelectedObjectName = *m_SelectedObjectNames.begin();
			}
			else
			{
				m_SelectedObjectName.clear();
			}
		}
		return;
	}

	m_SelectedObjectName = targetName;
	m_SelectedObjectNames.clear();
	m_SelectedObjectNames.insert(targetName);
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
	//텍스트 그리기전 리소스 해제
	ID3D11ShaderResourceView* nullSRV[128] = {};
	m_Engine.GetD3DDXDC()->PSSetShaderResources(0, 128, nullSRV);

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

	if (m_InputManager && m_GameViewport.HasViewportRect())
	{
		const ImVec2 rectMin = m_GameViewport.GetViewportRectMin();
		const ImVec2 rectMax = m_GameViewport.GetViewportRectMax();
		POINT clientMin{ static_cast<LONG>(rectMin.x), static_cast<LONG>(rectMin.y) };
		POINT clientMax{ static_cast<LONG>(rectMax.x), static_cast<LONG>(rectMax.y) };

		ScreenToClient(m_hwnd, &clientMin);
		ScreenToClient(m_hwnd, &clientMax);

		m_InputManager->SetViewportRect({ clientMin.x, clientMin.y, clientMax.x, clientMax.y });
	}

	HandleEditorViewportSelection();
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

		// Pause -> Play 는 저장되지 않도록
		ImGui::BeginDisabled(disablePlay);
		if (ImGui::Button("Play", ImVec2(buttonWidth, 0)))
		{
			if (m_EditorState == EditorPlayState::Stop)
			{
				m_SceneManager.SaveSceneToJson(m_CurrentScenePath);
			}
			m_GameManager->TurnReset();
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
			if (m_GameManager)
			{
				m_GameManager->TurnReset();
			}
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

		m_SelectedObjectNames.clear();
		m_SelectedObjectNames.insert(name);

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
	// 그룹 Select
	auto setPrimarySelection = [&](const std::string& name)
		{
			m_SelectedObjectName = name;
			m_SelectedObjectNames.clear();
			if (!name.empty())
			{
				m_SelectedObjectNames.insert(name);
			}
		};

	auto toggleSelection = [&](const std::string& name)
		{
			if (name.empty())
			{
				return;
			}

			const auto erased = m_SelectedObjectNames.erase(name);
			if (erased > 0)
			{
				if (m_SelectedObjectName == name)
				{
					if (!m_SelectedObjectNames.empty())
					{
						m_SelectedObjectName = *m_SelectedObjectNames.begin();
					}
					else
					{
						m_SelectedObjectName.clear();
					}
				}
				return;
			}

			m_SelectedObjectNames.insert(name);
			m_SelectedObjectName = name;
		};

	// copy
	const std::shared_ptr<GameObject>* selectedObject = nullptr;
	if (const auto it = objectLookup.find(m_SelectedObjectName); it != objectLookup.end())
	{
		selectedObject = &it->second;
	}

	if (!m_SelectedObjectName.empty() && m_SelectedObjectNames.empty())
	{
		if (objectLookup.find(m_SelectedObjectName) != objectLookup.end())
		{
			m_SelectedObjectNames.insert(m_SelectedObjectName);
		}
	}
	for (auto it = m_SelectedObjectNames.begin(); it != m_SelectedObjectNames.end();)
	{
		if (objectLookup.find(*it) == objectLookup.end())
		{
			it = m_SelectedObjectNames.erase(it);
		}
		else
		{
			++it;
		}
	}
	if (!m_SelectedObjectNames.empty() && m_SelectedObjectName.empty())
	{
		m_SelectedObjectName = *m_SelectedObjectNames.begin();
	}

	auto copySelectedObjects = [&](const std::vector<std::shared_ptr<GameObject>>& objects)
		{

			nlohmann::json clipboard = nlohmann::json::object();
			clipboard["objects"] = nlohmann::json::array();


			std::unordered_map<GameObject*, int> objectIds;
			int nextId = 0;
			for (const auto& root : objects)
			{
				if (!root)
				{
					continue;
				}
				std::string rootParentName;
				if (auto* rootTransform = root->GetComponent<TransformComponent>())
				{
					if (auto* rootParent = rootTransform->GetParent())
					{
						if (auto* rootParentOwner = dynamic_cast<GameObject*>(rootParent->GetOwner()))
						{
							if (m_SelectedObjectNames.find(rootParentOwner->GetName()) == m_SelectedObjectNames.end())
							{
								rootParentName = rootParentOwner->GetName();
							}
						}
					}
				}
					std::vector<GameObject*> stack;
					stack.push_back(root.get());
					while (!stack.empty())
					{
						GameObject* current = stack.back();
						stack.pop_back();

						if (!current)
						{
							continue;
						}
						const bool isRoot = (current == root.get());
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
	auto copySelectedObject = [&](const std::shared_ptr<GameObject>& object)
		{
			if (!object)
			{
				return;
			}
			copySelectedObjects({ object });
		};

	auto hasSelectedAncestor = [&](const std::shared_ptr<GameObject>& object) -> bool
		{
			if (!object)
			{
				return false;
			}
			auto* transform = object->GetComponent<TransformComponent>();
			auto* parent = transform ? transform->GetParent() : nullptr;
			while (parent)
			{
				auto* parentOwner = dynamic_cast<GameObject*>(parent->GetOwner());
				if (parentOwner && m_SelectedObjectNames.find(parentOwner->GetName()) != m_SelectedObjectNames.end())
				{
					return true;
				}
				parent = parent->GetParent();
			}
			return false;
		};

	auto gatherSelectedRoots = [&]() -> std::vector<std::shared_ptr<GameObject>>
		{
			std::vector<std::shared_ptr<GameObject>> roots;
			roots.reserve(m_SelectedObjectNames.size());
			for (const auto& name : m_SelectedObjectNames)
			{
				auto object = findObjectByName(name);
				if (!object)
				{
					continue;
				}
				if (hasSelectedAncestor(object))
				{
					continue;
				}
				roots.push_back(std::move(object));
			}
			return roots;
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
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
		{
			auto roots = gatherSelectedRoots();
			if (!roots.empty())
			{
				copySelectedObjects(roots);
			}
			else if (selectedObject && *selectedObject)
			{
				copySelectedObject(*selectedObject);
			}
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
		{
			pasteClipboardObject();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			if (!m_SelectedObjectNames.empty())
			{
				pendingDeletes.insert(pendingDeletes.end(), m_SelectedObjectNames.begin(), m_SelectedObjectNames.end());
			}
			else if (selectedObject && *selectedObject)
			{
				pendingDeletes.push_back((*selectedObject)->GetName());
			}
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

	std::vector<std::string> hierarchyOrder;
	hierarchyOrder.reserve(objectLookup.size());
	auto collectHierarchyOrder = [&](auto&& self, GameObject* object) -> void
		{
			if (!object)
			{
				return;
			}
			hierarchyOrder.push_back(object->GetName());
			auto* transform = object->GetComponent<TransformComponent>();
			if (!transform)
			{
				return;
			}
			for (auto* childTransform : transform->GetChildrens())
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
		};

	for (auto* root : rootObjects)
	{
		collectHierarchyOrder(collectHierarchyOrder, root);
	}

	std::unordered_map<std::string, size_t> hierarchyIndex;
	hierarchyIndex.reserve(hierarchyOrder.size());
	for (size_t i = 0; i < hierarchyOrder.size(); ++i)
	{
		hierarchyIndex[hierarchyOrder[i]] = i;
	}

	auto selectRange = [&](const std::string& startName, const std::string& endName)
		{
			const auto startIt = hierarchyIndex.find(startName);
			const auto endIt = hierarchyIndex.find(endName);
			if (startIt == hierarchyIndex.end() || endIt == hierarchyIndex.end())
			{
				setPrimarySelection(endName);
				return;
			}

			const size_t startIndex = startIt->second;
			const size_t endIndex = endIt->second;
			const size_t rangeStart = std::min(startIndex, endIndex);
			const size_t rangeEnd = (std::max)(startIndex, endIndex);

			m_SelectedObjectNames.clear();
			for (size_t i = rangeStart; i <= rangeEnd && i < hierarchyOrder.size(); ++i)
			{
				m_SelectedObjectNames.insert(hierarchyOrder[i]);
			}
			m_SelectedObjectName = endName;
		};

	auto handleSelectionClick = [&](const std::string& name, ImGuiIO& io)
		{
			if (io.KeyShift && !m_SelectedObjectName.empty())
			{
				selectRange(m_SelectedObjectName, name);
			}
			else if (io.KeyCtrl)
			{
				toggleSelection(name);
			}
			else if (m_SelectedObjectNames.size() > 1
				&& m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end())
			{
				m_SelectedObjectName = name;
			}
			else
			{
				setPrimarySelection(name);
			}
		};

	auto splitDragPayloadNames = [&](const char* payloadData) -> std::vector<std::string>
		{
			std::vector<std::string> names;
			if (!payloadData)
			{
				return names;
			}
			std::string data(payloadData);
			size_t start = 0;
			while (start <= data.size())
			{
				const size_t end = data.find('\n', start);
				const size_t length = (end == std::string::npos) ? data.size() - start : end - start;
				if (length > 0)
				{
					names.emplace_back(data.substr(start, length));
				}
				if (end == std::string::npos)
				{
					break;
				}
				start = end + 1;
			}
			return names;
		};

	auto drawHierarchyNode = [&](auto&& self, GameObject* object) -> void
		{
			if (!object) { return; }
			const std::string& name = object->GetName();
			auto* transform = object->GetComponent<TransformComponent>();
			auto* children = transform ? &transform->GetChildrens() : nullptr;
			const bool hasChildren = children && !children->empty();
			ImGuiIO& io = ImGui::GetIO();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
			if (m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end())
			{
				flags |= ImGuiTreeNodeFlags_Selected;
			}

			ImGui::PushID(object);

			if (!hasChildren) {
				flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

				ImGui::TreeNodeEx(name.c_str(), flags);
				if (ImGui::IsItemClicked())
				{
					handleSelectionClick(name, io);
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					setPrimarySelection(name);
					auto selected = findObjectByName(name);
					if (selected)
					{
						FocusEditorCameraOnObject(selected);
					}
				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
				{
					if (m_SelectedObjectNames.find(name) == m_SelectedObjectNames.end() && !io.KeyCtrl)
					{
						setPrimarySelection(name);
					}
				}

				if (ImGui::BeginPopupContextItem("ObjectContext"))
				{
					auto selected = findObjectByName(name);
					if (ImGui::MenuItem("Copy") && selected)
					{
						auto roots = gatherSelectedRoots();
						if (!roots.empty())
						{
							copySelectedObjects(roots);
						}
						else
						{
							copySelectedObject(selected);
						}
					}
					if (ImGui::MenuItem("Duplicate") && selected)
					{
						auto roots = gatherSelectedRoots();
						if (!roots.empty())
						{
							copySelectedObjects(roots);
						}
						else
						{
							copySelectedObject(selected);
						}

						if (m_ObjectClipboardHasData)
						{
							queuePasteObject(m_ObjectClipboard, "Duplicate GameObject");
						}
					}
					if (ImGui::MenuItem("Delete"))
					{
						if (m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end() && m_SelectedObjectNames.size() > 1)
						{
							pendingDeletes.insert(pendingDeletes.end(), m_SelectedObjectNames.begin(), m_SelectedObjectNames.end());
						}
						else
						{
							pendingDeletes.push_back(name);
						}
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					std::string payloadNames;
					const bool isMulti = m_SelectedObjectNames.size() > 1
						&& m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end();
					if (isMulti)
					{
						auto roots = gatherSelectedRoots();
						for (size_t i = 0; i < roots.size(); ++i)
						{
							if (!roots[i])
							{
								continue;
							}
							if (!payloadNames.empty())
							{
								payloadNames.push_back('\n');
							}
							payloadNames += roots[i]->GetName();
						}
						ImGui::SetDragDropPayload("HIERARCHY_OBJECTS", payloadNames.c_str(), payloadNames.size() + 1);
					}
					else
					{
						ImGui::SetDragDropPayload("HIERARCHY_OBJECT", name.c_str(), name.size() + 1);
					}
					ImGui::TextUnformatted(name.c_str());
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECTS"))
					{
						const char* payloadData = static_cast<const char*>(payload->Data);
						auto names = splitDragPayloadNames(payloadData);
						for (const auto& childName : names)
						{
							reparentObject(childName, name);
						}
					}
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
					handleSelectionClick(name, io);
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					setPrimarySelection(name);
					auto selected = findObjectByName(name);
					if (selected)
					{
						FocusEditorCameraOnObject(selected);
					}
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
				{
					if (m_SelectedObjectNames.find(name) == m_SelectedObjectNames.end() && !io.KeyCtrl)
					{
						setPrimarySelection(name);
					}
				}
				if (ImGui::BeginPopupContextItem("ObjectContext"))
				{
					auto selected = findObjectByName(name);
					if (ImGui::MenuItem("Copy") && selected)
					{
						auto roots = gatherSelectedRoots();
						if (!roots.empty())
						{
							copySelectedObjects(roots);
						}
						else
						{
							copySelectedObject(selected);
						}
					}
					if (ImGui::MenuItem("Duplicate") && selected)
					{
						auto roots = gatherSelectedRoots();
						if (!roots.empty())
						{
							copySelectedObjects(roots);
						}
						else
						{
							copySelectedObject(selected);
						}
						if (m_ObjectClipboardHasData)
						{
							queuePasteObject(m_ObjectClipboard, "Duplicate GameObject");
						}
					}
					if (ImGui::MenuItem("Delete"))
					{
						if (m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end() && m_SelectedObjectNames.size() > 1)
						{
							pendingDeletes.insert(pendingDeletes.end(), m_SelectedObjectNames.begin(), m_SelectedObjectNames.end());
						}
						else
						{
							pendingDeletes.push_back(name);
						}
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					std::string payloadNames;
					const bool isMulti = m_SelectedObjectNames.size() > 1
						&& m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end();
					if (isMulti)
					{
						auto roots = gatherSelectedRoots();
						for (size_t i = 0; i < roots.size(); ++i)
						{
							if (!roots[i])
							{
								continue;
							}
							if (!payloadNames.empty())
							{
								payloadNames.push_back('\n');
							}
							payloadNames += roots[i]->GetName();
						}
						ImGui::SetDragDropPayload("HIERARCHY_OBJECTS", payloadNames.c_str(), payloadNames.size() + 1);
					}
					else
					{
						ImGui::SetDragDropPayload("HIERARCHY_OBJECT", name.c_str(), name.size() + 1);
					}
					ImGui::TextUnformatted(name.c_str());
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECTS"))
					{
						const char* payloadData = static_cast<const char*>(payload->Data);
						auto names = splitDragPayloadNames(payloadData);
						for (const auto& childName : names)
						{
							reparentObject(childName, name);
						}
					}
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
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECTS"))
			{
				const char* payloadData = static_cast<const char*>(payload->Data);
				auto names = splitDragPayloadNames(payloadData);
				for (const auto& childName : names)
				{
					detachObject(childName);
				}
			}
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
				m_SelectedObjectNames.clear();
				m_SelectedObjectNames.insert(uniqueName);
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
			m_SelectedObjectNames.clear();
			m_SelectedObjectNames.insert(uniqueName);
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
		m_SelectedObjectNames.erase(name);
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

	if (m_SelectedObjectName.empty() && !m_SelectedObjectNames.empty())
	{
		m_SelectedObjectName = *m_SelectedObjectNames.begin();
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

	if (m_SelectedObjectNames.size() > 1)
	{
		ImGui::Text("Multiple objects selected");
		ImGui::End();
		return;
	}

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
				m_SelectedObjectNames.erase(oldName);
				m_SelectedObjectNames.insert(newName);
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
						m_SelectedObjectNames.erase(newName);
						m_SelectedObjectNames.insert(oldName);
						m_LastSelectedObjectName = oldName;
						CopyStringToBuffer(oldName, m_ObjectNameBuffer);
					},
					[this, scenePtr, oldName, newName]()
					{
						if (!scenePtr)
							return;
						scenePtr->RenameGameObject(oldName, newName);
						m_SelectedObjectName = newName;
						m_SelectedObjectNames.erase(oldName);
						m_SelectedObjectNames.insert(newName);
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
			if (typeName == "LightComponent" || typeName == "AnimationComponent")
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
				m_SelectedObjectNames.clear();
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

					if (isSceneFile) {
						m_SceneManager.RegisterSceneFromJson(entry.path()); //Scene 모두 등록
					}
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
								m_SelectedObjectNames.clear();
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
										m_SelectedObjectNames.clear();
										if (!m_SelectedObjectName.empty())
										{
											m_SelectedObjectNames.insert(m_SelectedObjectName);
										}
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
										m_SelectedObjectNames.clear();
										if (!m_SelectedObjectName.empty())
										{
											m_SelectedObjectNames.insert(m_SelectedObjectName);
										}
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
	m_SelectedObjectNames.clear();
	if (!m_SelectedObjectName.empty())
	{
		m_SelectedObjectNames.insert(m_SelectedObjectName);
	}
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
