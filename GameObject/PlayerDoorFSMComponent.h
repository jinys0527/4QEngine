#pragma once
#include "FSMComponent.h"

class PlayerDoorFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerDoorFSMComponent";
	const char* GetTypeName() const override;

	PlayerDoorFSMComponent();
	virtual ~PlayerDoorFSMComponent() override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

private:
	void ResolveDoorVerdictNow();
	bool  m_WaitingForDoorRollConfirm = false;
	bool  m_DoorRollAnimationObserved = false;
	float m_DoorRollWaitTimer = 0.0f;
	bool  m_ListenersRegistered = false;
	bool  m_PendingDoorVerdict = false;
	bool  m_RollPhaseLocked = false;
};