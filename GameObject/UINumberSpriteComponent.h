#pragma once
#include "UIComponent.h"
#include "ResourceHandle.h"
#include "UIPrimitives.h"
#include <DirectXMath.h>
#include <array>
#include <string>
#include <vector>

class EventDispatcher;
class UIManager;
class UIObject;
class Scene;

class UINumberSpriteComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UINumberSpriteComponent";
	~UINumberSpriteComponent() override;
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

	void SetDigitTextureHandle(int digit, const TextureHandle& handle);
	const TextureHandle& GetDigitTextureHandle(int digit) const;
	void SetDigitTextures(const std::array<TextureHandle, 10>& textures);
	const std::array<TextureHandle, 10>& GetDigitTextures() const { return m_DigitTextures; }

	void SetDigitSpacing(const float& spacing);
	const float& GetDigitSpacing() const { return m_DigitSpacing; }
	void SetDigitOffsets(std::vector<UIAnchor> offsets);
	const std::vector<UIAnchor>& GetDigitOffsets() const { return m_DigitOffsets; }
	void SetPerDigitAdvance(const std::array<float, 10>& offsets);
	const std::array<float, 10>& GetPerDigitAdvance() const { return m_PerDigitAdvance; }
	void SetFixedDigitCount(const int& count);
	const int& GetFixedDigitCount() const { return m_FixedDigitCount; }
	void SetDigitTintColor(const DirectX::XMFLOAT4& color);
	const DirectX::XMFLOAT4& GetDigitTintColor() const { return m_DigitTintColor; }

	void SetAutoValueSource(const int& source);
	const int& GetAutoValueSource() const { return m_AutoValueSource; }
	void SetAutoValueActorId(const int& actorId);
	const int& GetAutoValueActorId() const { return m_AutoValueActorId; }

	void SetUseAsCombatPopup(const bool& useAsPopup);
	const bool& GetUseAsCombatPopup() const { return m_UseAsCombatPopup; }
	void SetPopupObjectNames(std::vector<std::string> names);
	const std::vector<std::string>& GetPopupObjectNames() const { return m_PopupObjectNames; }
	void SetPopupTrackActorId(const int& actorId);
	const int& GetPopupTrackActorId() const { return m_PopupTrackActorId; }
	void SetPopupRiseDistance(const float& rise);
	const float& GetPopupRiseDistance() const { return m_PopupRiseDistance; }
	void SetPopupLifetime(const float& time);
	const float& GetPopupLifetime() const { return m_PopupLifetime; }
	void SetPopupFadeOutTime(const float& time);
	const float& GetPopupFadeOutTime() const { return m_PopupFadeOutTime; }
	void SetDamageTint(const DirectX::XMFLOAT4& color);
	const DirectX::XMFLOAT4& GetDamageTint() const { return m_DamageTint; }
	void SetHealTint(const DirectX::XMFLOAT4& color);
	const DirectX::XMFLOAT4& GetHealTint() const { return m_HealTint; }
	void SetDealTint(const DirectX::XMFLOAT4& color);
	const DirectX::XMFLOAT4& GetDealTint() const { return m_DealTint; }
	void SetCriticalTint(const DirectX::XMFLOAT4& color);
	const DirectX::XMFLOAT4& GetCriticalTint() const { return m_CriticalTint; }
	void SetGoldTint(const DirectX::XMFLOAT4& color);
	const DirectX::XMFLOAT4& GetGoldTint() const { return m_GoldTint; }

	void RefreshVisuals();

private:
	struct PopupState
	{
		std::string objectName;
		float elapsed = 0.0f;
		bool  active = false;
		UIRect baseBounds{};
	};

	UIObject*  FindUIObject(const std::string& name) const;
	bool       EnsureDigitTargetsResolved();
	void       ApplyValue();
	void       UpdatePopupPool();
	void       TickPopups(float deltaTime);
	void       ShowPopup(int value, const DirectX::XMFLOAT4& tint);
	void       TryInitializePopupPool();
	bool       TryPrepareRuntimeBindings();
	bool       ArePopupTargetsReady() const;
	bool       TryUpdateValueFromAutoSource();

	int   m_Value = 0;
	bool  m_Enabled = true;
	bool  m_LeadingZero = false;
	bool  m_ValueDirty = true;
	std::vector<UIObject*>        m_DigitTargets;
	std::array<TextureHandle, 10> m_DigitTextures{};
	float m_DigitSpacing = 0.0f;
	std::vector<UIAnchor> m_DigitOffsets;
	std::array<float, 10> m_PerDigitAdvance{};
	int   m_FixedDigitCount = 0;
	DirectX::XMFLOAT4 m_DigitTintColor{ 1, 1, 1, 1 };
	int m_AutoValueSource = 0;
	int m_AutoValueActorId = 0;

	bool  m_UseAsCombatPopup = false;
	std::vector<std::string> m_PopupObjectNames;
	int   m_PopupTrackActorId = 0;
	float m_PopupRiseDistance = 40.0f;
	float m_PopupLifetime = 0.6f;
	float m_PopupFadeOutTime = 0.25f;
	DirectX::XMFLOAT4 m_DamageTint  { 1.0f,  0.3f,  0.3f,  1.0f };
	DirectX::XMFLOAT4 m_HealTint    { 0.3f,  1.0f,  0.3f,  1.0f };
	DirectX::XMFLOAT4 m_DealTint    { 1.0f,  0.9f,  0.25f, 1.0f };
	DirectX::XMFLOAT4 m_CriticalTint{ 0.75f, 0.75f, 0.75f, 1.0f };
	DirectX::XMFLOAT4 m_GoldTint	{ 1.0f,  0.85f, 0.2f,  1.0f };
	std::vector<PopupState> m_PopupStates;
	bool  m_PopupPoolDirty = true;
	std::vector<UIRect>     m_BaseDigitBounds;
	EventDispatcher*        m_Dispatcher = nullptr;
	bool  m_RuntimeBindingsReady = false;
	bool  m_ListenerRegistered = false;
};

