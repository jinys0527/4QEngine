#include "pch.h"
#include "ClientScene.h"
#include "GameObject.h"
#include "CameraObject.h"
#include "TransformComponent.h"




void ClientScene::Initialize()
{

}

void ClientScene::Finalize()
{

}

void ClientScene::Leave()
{

}

void ClientScene::FixedUpdate()
{
	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (gameObject)
		{
			gameObject->FixedUpdate();
		}
	}
}

void ClientScene::Update(float dTime)
{
	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (gameObject)
		{
			gameObject->Update(dTime);
		}
	}
}

void ClientScene::StateUpdate(float dTime)
{
	Scene::StateUpdate(dTime);
}

