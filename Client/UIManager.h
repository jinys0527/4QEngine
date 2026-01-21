#pragma once
#include "IEventListener.h"
#include <vector>
#include <memory>
#include <windows.h>
//#include "RenderData.h"
#include "UIObject.h"
#include "EventDispatcher.h"
#include "json.hpp"

class UIManager : public IEventListener
{
public:
	UIManager() {}
	virtual ~UIManager();

	void AddUI(std::string sceneName, std::shared_ptr<UIObject> uiObject)
	{
		m_UIObjects[sceneName][uiObject->m_Name] = uiObject;
	}

	void RemoveUI(std::string sceneName, std::shared_ptr<UIObject> uiObject)
	{
		auto it = m_UIObjects.find(sceneName);
		if (it != m_UIObjects.end())
		{
			auto& uiMap = it->second;
			auto it2 = uiMap.find(uiObject->m_Name);
			if (it2 != uiMap.end())
			{
				uiMap.erase(it2);
				if (uiMap.empty())  // 씬 내 UI가 다 지워지면 씬 키도 지울 수 있음 (선택 사항)
				{
					m_UIObjects.erase(it);
				}
			}
		}
	}

	void OnEvent(EventType type, const void* data) override;

	bool IsFullScreenUIActive() const;

	void Update(float deltaTime);

	//void Render(std::vector<UIRenderInfo>& renderInfo, std::vector<UITextInfo>& textInfo);

	void SendEventToUI(UIObject* ui, EventType type, const void* data);

	void Start();

	void Reset();

	void SetEventDispatcher(EventDispatcher* eventDispatcher);

	void SetCurrentScene(std::string currentSceneName) 
	{
		m_CurrentSceneName = currentSceneName;
	}

	std::string GetCurrentScene() const
	{
		return m_CurrentSceneName;
	}

	std::unordered_map <std::string, std::unordered_map<std::string, std::shared_ptr<UIObject>>>& GetUIObjects()
	{
		return m_UIObjects;
	}
	// UIManager: UI가 변경될 때 호출하는 함수
	void UpdateSortedUI(const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap);

	void RefreshUIListForCurrentScene();

	void SerializeSceneUI(const std::string& sceneName, nlohmann::json& out) const;
	void DeSerializeSceneUI(const std::string& sceneName, const nlohmann::json& data);

private:
	// UIManager 멤버 변수에 추가 (헤더에 선언)
	std::vector<UIObject*> m_SortedUI;

	UIObject* m_ActiveUI;
	UIObject* m_LastHoveredUI = nullptr;
	bool m_FullScreenUIActive = false;
	int m_FullScreenZ = -1;
	EventDispatcher* m_EventDispatcher; 
	std::string m_CurrentSceneName;
	void DispatchToTopUI(EventType type, const void* data);
	std::unordered_map <std::string, std::unordered_map<std::string, std::shared_ptr<UIObject>>> m_UIObjects;
};

