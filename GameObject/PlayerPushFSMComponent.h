#pragma once
#include "FSMComponent.h"

class PlayerPushFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerPushFSMComponent";
	const char* GetTypeName() const override;

	PlayerPushFSMComponent();
	~PlayerPushFSMComponent() override = default;

	void Start() override;
};