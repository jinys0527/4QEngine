#pragma once
#include "UIComponent.h"
#include "UIDicePanelTypes.h"
#include <string>
#include <vector>

class UIManager;
class UIObject;
class Scene;

class UIDiceDisplayComponent;
class UIDiceRollAnimationComponent;

class UIDicePanelComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIDicePanelComponent";
	const char* GetTypeName() const override;

	void Start  () override;
	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void		SetEnabled(const bool& enabled);
	const bool& GetEnabled() const { return m_Enabled; }

	void SetSlots(std::vector<UIDicePanelSlot> slots);
	const std::vector<UIDicePanelSlot>& GetSlots() const { return m_Slots; }

	void			   SetActiveDiceType(const std::string& type);
	const std::string& GetActiveDiceType() const { return m_ActiveDiceType; }

	void		SetAutoVisibility(const bool& enabled);
	const bool& GetAutoVisibility() const { return m_AutoVisibility; }

	void RefreshBindings();

private:
	UIManager* GetUIManager() const;
	Scene*     GetScene() const;
	UIObject*  FindUIObject(const std::string& name) const;
	void ApplySlot(UIObject& object, const UIDicePanelSlot& slot) const;

	std::vector<UIDicePanelSlot> m_Slots;
	std::string m_ActiveDiceType;
	bool m_Enabled = true;
	bool m_AutoVisibility = true;
	bool m_BindingsDirty = true;
	mutable UIManager* m_UIManager = nullptr;
};

