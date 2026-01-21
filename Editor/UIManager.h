#pragma once
#include "IEventListener.h"
#include <vector>
#include <memory>
#include <windows.h>
#include <functional>
//#include "RenderData.h"
#include "UIObject.h"
#include "json.hpp"

class EventDispatcher;
struct HorizontalBoxSlot;

class UIManager : public IEventListener
{
public:
	UIManager() = default;
	virtual ~UIManager();

	void SetEventDispatcher(EventDispatcher* eventDispatcher);

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
				if (uiObject && m_ActiveUI == uiObject.get())
				{
					m_ActiveUI = nullptr;
				}
				if (uiObject && m_LastHoveredUI == uiObject.get())
				{
					m_LastHoveredUI = nullptr;
				}
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

	std::shared_ptr<UIObject> FindUIObject(const std::string& sceneName, const std::string& objectName);

	template<typename ComponentType, typename Func>
	bool ApplyToComponent(const std::string& sceneName, const std::string& objectName, Func&& func)
	{
		auto uiObject = FindUIObject(sceneName, objectName);
		if (!uiObject)
		{
			return false;
		}

		auto* component = uiObject->GetComponent<ComponentType>();
		if (!component)
		{
			return false;
		}

		func(*component);
		return true;
	}

	// UIManager: UI가 변경될 때 호출하는 함수
	void UpdateSortedUI(const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap);

	void RefreshUIListForCurrentScene();

	bool RegisterButtonOnClicked(const std::string& sceneName, const std::string& objectName, std::function<void()> callback);
	bool ClearButtonOnClicked(const std::string& sceneName, const std::string& objectName);
	bool RegisterHorizontalSlot(const std::string& sceneName, const std::string& horizontalName, const std::string& childName, const HorizontalBoxSlot& slot);
	bool RemoveHorizontalSlot(const std::string& sceneName, const std::string& horizontalName, const std::string& childName);
	bool ClearHorizontalSlots(const std::string& sceneName, const std::string& horizontalName);
	bool ApplyHorizontalLayout(const std::string& sceneName, const std::string& horizontalName);

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

