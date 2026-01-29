#pragma once
#include "FSMComponent.h"

class PlayerMoveFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerMoveFSMComponent";
	const char* GetTypeName() const override;

	PlayerMoveFSMComponent();
	~PlayerMoveFSMComponent() override = default;

	void Start() override;
};