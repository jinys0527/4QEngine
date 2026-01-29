#pragma once
#include "FSMComponent.h"

class PlayerFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerFSMComponent";
	const char* GetTypeName() const override;

	PlayerFSMComponent();
	~PlayerFSMComponent() override = default;

	void Start() override;
};

