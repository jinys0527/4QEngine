#pragma once
#include "FSMComponent.h"

class AnimFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "AnimFSMComponent";
	const char* GetTypeName() const override;

	AnimFSMComponent();

	void Notify(const std::string& notifyName);
};

