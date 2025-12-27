#include "ButtonUI.h"
#include "Component.h"

ButtonUI::ButtonUI(EventDispatcher& eventDispatcher) : UIObject(eventDispatcher)
{
	m_UI = AddComponent<UIButtonComponent>();
}
