#pragma once
#include "UIObject.h"
#include "UIButtonComponent.h"

class ButtonUI : public UIObject
{
public:
	ButtonUI(EventDispatcher& eventDispatcher);
	virtual ~ButtonUI() = default;

private:
	UIButtonComponent* m_UI;
};

