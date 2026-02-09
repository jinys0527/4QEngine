#pragma once
#include "UIComponent.h"
#include "ResourceHandle.h"
#include <array>
#include <string>
#include <vector>

class UIManager;
class UIObject;
class Scene;

class UINumberSpriteComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UINumberSpriteComponent";
	const char* GetTypeName() const override;

	void Start  () override;
	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void		SetEnabled(const bool& enabled);
	const bool& GetEnabled() const { return m_Enabled; }

	void	   SetValue(const int& value);
	const int& GetValue() const { return m_Value; }

	void		SetLeadingZero(const bool& leadingZero);
	const bool& GetLeadingZero() const { return m_LeadingZero; }

	void SetDigitObjectNames(std::vector<std::string> names);
	const std::vector<std::string>& GetDigitObjectNames() const { return m_DigitObjectNames; }

	void SetDigitTextureHandle(int digit, const TextureHandle& handle);
	const TextureHandle& GetDigitTextureHandle(int digit) const;
	void SetDigitTextures(const std::array<TextureHandle, 10>& textures);
	const std::array<TextureHandle, 10>& GetDigitTextures() const { return m_DigitTextures; }

	void RefreshVisuals();

private:
	UIManager* GetUIManager() const;
	Scene*     GetScene() const;
	UIObject*  FindUIObject(const std::string& name) const;
	void       ApplyValue();

	int  m_Value = 0;
	bool m_Enabled = true;
	bool m_LeadingZero = false;
	bool m_ValueDirty = true;
	std::vector<std::string>	  m_DigitObjectNames;
	std::array<TextureHandle, 10> m_DigitTextures{};
	mutable UIManager* m_UIManager = nullptr;
};

