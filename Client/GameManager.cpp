#include "pch.h"
#include <iostream>
#include "GameManager.h"

GameManager::GameManager(EventDispatcher& eventDispatcher) : m_EventDispatcher(eventDispatcher)
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