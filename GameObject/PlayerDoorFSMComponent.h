#pragma once
#include "FSMComponent.h"

class PlayerDoorFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerDoorFSMComponent";
	const char* GetTypeName() const override;

	PlayerDoorFSMComponent();
	~PlayerDoorFSMComponent() override = default;

	void Start() override;
};