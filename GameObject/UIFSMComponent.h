#pragma once
#include "FSMComponent.h"

class UIFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "UIFSMComponent";
	const char* GetTypeName() const override;

	UIFSMComponent();
	~UIFSMComponent() override;

	void Start() override;

protected:
	std::optional<std::string> TranslateEvent(EventType type, const void* data) override;
};

