#pragma once
#include "FSMComponent.h"

class PlayerShopFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerShopFSMComponent";
	const char* GetTypeName() const override;

	PlayerShopFSMComponent();
	virtual ~PlayerShopFSMComponent() override = default;

	void Start() override;
};