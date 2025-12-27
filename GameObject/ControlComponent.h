#pragma once
#include "Component.h"
#include "IEventListener.h"

class ControlComponent : public Component, public IEventListener
{
public:
	ControlComponent();
	virtual ~ControlComponent();

	static constexpr const char* StaticTypeName = "ControlComponent";
	const char* GetTypeName() const override { return StaticTypeName; }


	void Start();
	void Update(float deltaTime) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void OnEvent(EventType type, const void* data) override;

	void RegisterKeyDownCallback(int vkKey, std::function<void()> callback);
	void UnregisterKeyDownCallback(int vkKey);

	void Enter();
	void Leave();

	void RegisterAnyKeyCallback(std::function<void()> callback)
	{
		m_AnyKeyCallback = callback;
	}

private:
	std::function<void()> m_AnyKeyCallback;
	std::string m_CurrentSceneName;
	std::unordered_map<int, std::function<void()>> m_KeyDownCallbacks;

	bool m_IsCorrectScene = false;
};

