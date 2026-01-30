#pragma once
#include "FSMComponent.h"

class PlayerCombatFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerCombatFSMComponent";
	const char* GetTypeName() const override;

	PlayerCombatFSMComponent();
	virtual ~PlayerCombatFSMComponent() override = default;

	void Start() override;
};

