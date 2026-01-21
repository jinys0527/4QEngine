#include "pch.h"
#include "DefaultScene.h"
#include "CameraObject.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "DirectionalLightComponent.h"
#include "MeshRenderer.h"


void DefaultScene::Initialize()
{
	auto gamecamera = std::make_shared<CameraObject>(GetEventDispatcher(), 1280.0f, 720.0f);

	gamecamera->SetName("Main Camera");
	SetGameCamera(gamecamera); // Main Camera
	AddGameObject(gamecamera);
	

	//editorCam
	auto editorCamera = std::make_shared<CameraObject>(GetEventDispatcher(), 1280.0f, 720.0f);
	editorCamera->SetName("Editor Camera");
	SetEditorCamera(editorCamera);
	AddGameObject(editorCamera);
	if (auto* editorCameraComponent = editorCamera->GetComponent<CameraComponent>())
	{
		editorCameraComponent->SetNearZ(0.1f);
		editorCameraComponent->SetFarZ(1000.0f);
	}

	//초기 카메라 위치 Set
	m_GameCamera->GetComponent<TransformComponent>()->SetPosition(XMFLOAT3{ 0.0f,4.0f,-11.0f });
	m_EditorCamera->GetComponent<TransformComponent>()->SetPosition(XMFLOAT3{ 0.0f,4.0f,-11.0f });

	m_GameCamera->GetComponent<TransformComponent>()->SetRotationEuler(XMFLOAT3{ 15.0f,0.0f,0.0f });
	m_EditorCamera->GetComponent<TransformComponent>()->SetRotationEuler(XMFLOAT3{ 15.0f,0.0f,0.0f });
	

	auto lightObject = std::make_shared<GameObject>(GetEventDispatcher());
	lightObject->SetName("DirectionalLight");
	if (auto* light = lightObject->AddComponent<DirectionalLightComponent>())
	{
		light->SetColor({ 1.0f, 1.0f, 1.0f });
		light->SetIntensity(1.0f);
		light->SetDirection({ 0.0f, -1.0f, 0.0f });
	}
	AddGameObject(lightObject); // Light는 TransparentObjects로 등록
}

void DefaultScene::Finalize()
{
}


void DefaultScene::Leave()
{
}

void DefaultScene::FixedUpdate()
{

	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (gameObject)
		{
			gameObject->FixedUpdate();
		}
	}
}

void DefaultScene::Update(float deltaTime)
{	
	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (gameObject)
		{
			gameObject->Update(deltaTime);
		}
	}
}

void DefaultScene::StateUpdate(float deltaTime)
{
	if (m_EditorCamera)
	{
		m_EditorCamera->Update(deltaTime);
	}
}

