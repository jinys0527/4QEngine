#include "pch.h"
#include <iostream>
#include "GameManager.h"

GameManager::GameManager()
{

}

GameManager::~GameManager()
{
}

void GameManager::Reset()
{
}

void GameManager::OnEvent(EventType type, const void* data)
{

}

void GameManager::Initial()
{

}

void GameManager::RequestSceneChange(const std::string& name)
{
	if (!m_EventDispatcher)
	{
		return;
	}

	Events::SceneChangeRequest request{ name };
	m_EventDispatcher->Dispatch(EventType::SceneChangeRequested, &request);
}
