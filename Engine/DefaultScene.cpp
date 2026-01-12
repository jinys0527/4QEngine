#include "pch.h"
#include "DefaultScene.h"
#include "CameraObject.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "TransformComponent.h"

void DefaultScene::Initialize()
{
	auto gamecamera = std::make_shared<CameraObject>(m_EventDispatcher, 1280.0f, 720.0f);

	gamecamera->SetName("Main Camera");
	SetGameCamera(gamecamera); // Main Camera

	auto editorCamera = std::make_shared<CameraObject>(m_EventDispatcher, 1280.0f, 720.0f);
	editorCamera->SetName("Editor Camera");
	SetEditorCamera(editorCamera);


	auto lightObject = std::make_shared<GameObject>(m_EventDispatcher);
	lightObject->SetName("DirectionalLight");
	lightObject->AddComponent<TransformComponent>();
	if (auto* light = lightObject->AddComponent<LightComponent>())
	{
		light->SetType(RenderData::LightType::Directional);
		light->SetColor({ 1.0f, 1.0f, 1.0f });
		light->SetIntensity(1.0f);
		light->SetDirection({ 0.0f, -1.0f, 0.0f });
	}
	AddGameObject(lightObject,false); // Light는 TransparentObjects로 등록
}

void DefaultScene::Finalize()
{
}

void DefaultScene::Enter()
{
}

void DefaultScene::Leave()
{
}

void DefaultScene::FixedUpdate()
{

	for (const auto& [name, gameObject] : m_OpaqueObjects)
	{
		if (gameObject)
		{
			gameObject->FixedUpdate();
		}
	}

	for (const auto& [name, gameObject] : m_TransparentObjects)
	{
		if (gameObject)
		{
			gameObject->FixedUpdate();
		}
	}

}

void DefaultScene::Update(float deltaTime)
{	// ★★★★★★★★★★★★★★★★★★★★★★★★★
	// Object Update를 따로 하면 문제 생길 수 있음 
	// 투명 끝나고 -> 불투명 Update 하면. Logic에서 문제 생길 수 도 
	// 문제 생기는 경우
	// Upcasting 하던가, GameObject 자체에 멤버로 투명 불투병 bool 갖고, 이거 따라서 분류해서 Render 주던가

	if (m_EditorCamera)
	{
		m_EditorCamera->Update(deltaTime);
	}


	for (const auto& [name, gameObject] : m_OpaqueObjects)
	{
		if (gameObject)
		{
			gameObject->Update(deltaTime);
		}
	}

	for (const auto& [name, gameObject] : m_TransparentObjects)
	{
		if (gameObject)
		{
			gameObject->Update(deltaTime);
		}
	}

}

