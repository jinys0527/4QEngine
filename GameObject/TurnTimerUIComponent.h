#pragma once

#include "UIComponent.h"

class EventDispatcher;

class TurnTimerUIComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "TurnTimerUIComponent";
	~TurnTimerUIComponent() override;
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetEnabled(const bool& enabled);
	const bool& GetEnabled() const { return m_Enabled; }

	void SetUpdateNumberSprite(const bool& enabled);
	const bool& GetUpdateNumberSprite() const { return m_UpdateNumberSprite; }

	void SetHideWhenInactive(const bool& enabled);
	const bool& GetHideWhenInactive() const { return m_HideWhenInactive; }

	void SetRoundUpSeconds(const bool& enabled);
	const bool& GetRoundUpSeconds() const { return m_RoundUpSeconds; }

private:
	bool TryPrepareRuntimeBindings();
	void DetachFromDispatcher();
	void ApplyTimerVisuals(float remainingSeconds, float remainingRatio, bool isPlayerTurnActive);

	EventDispatcher* m_Dispatcher = nullptr;
	bool m_ListenerRegistered = false;
	bool m_RuntimeBindingsReady = false;

	bool m_Enabled = true;
	bool m_UpdateNumberSprite = true;
	bool m_HideWhenInactive = false;
	bool m_RoundUpSeconds = true;
};