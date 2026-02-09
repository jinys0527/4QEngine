#include "UINumberSpriteComponent.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"
#include "UIObject.h"
#include "UIImageComponent.h"
#include <algorithm>

REGISTER_UI_COMPONENT(UINumberSpriteComponent)
REGISTER_PROPERTY(UINumberSpriteComponent, Enabled)
REGISTER_PROPERTY(UINumberSpriteComponent, Value)
REGISTER_PROPERTY(UINumberSpriteComponent, LeadingZero)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitObjectNames)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitTextures)

void UINumberSpriteComponent::Start()
{
	if (auto* scene = GetScene())
	{
		auto& services = scene->GetServices();
		if (services.Has<UIManager>())
		{
			m_UIManager = &services.Get<UIManager>();
		}
	}

	m_ValueDirty = true;
}

void UINumberSpriteComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
	(void)deltaTime;

	if (!m_Enabled)
	{
		return;
	}

	if (!m_ValueDirty)
	{
		return;
	}

	ApplyValue();
	m_ValueDirty = false;
}

void UINumberSpriteComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
	(void)type;
	(void)data;
}

void UINumberSpriteComponent::SetEnabled(const bool& enabled)
{
	if (m_Enabled == enabled)
	{
		return;
	}

	m_Enabled = enabled;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetValue(const int& value)
{
	const int clamped = max(0, value);
	if (m_Value == clamped)
	{
		return;
	}

	m_Value = value;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetLeadingZero(const bool& leadingZero)
{
	if (m_LeadingZero == leadingZero)
	{
		return;
	}

	m_LeadingZero = leadingZero;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitObjectNames(std::vector<std::string> names)
{
	m_DigitObjectNames = std::move(names);
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitTextureHandle(int digit, const TextureHandle& handle)
{
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return;
	}

	m_DigitTextures[static_cast<size_t>(digit)] = handle;
	m_ValueDirty = true;
}

const TextureHandle& UINumberSpriteComponent::GetDigitTextureHandle(int digit) const
{
	static TextureHandle invalid = TextureHandle::Invalid();
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return invalid;
	}
	return m_DigitTextures[static_cast<size_t>(digit)];
}

void UINumberSpriteComponent::SetDigitTextures(const std::array<TextureHandle, 10>& textures)
{
	m_DigitTextures = textures;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::RefreshVisuals()
{
	m_ValueDirty = true;
}

UIManager* UINumberSpriteComponent::GetUIManager() const
{
	if (m_UIManager)
	{
		return m_UIManager;
	}

	if (auto* scene = GetScene())
	{
		auto& services = scene->GetServices();
		if (services.Has<UIManager>())
		{
			m_UIManager = &services.Get<UIManager>();
		}
	}

	return m_UIManager;
}

Scene* UINumberSpriteComponent::GetScene() const
{
	auto* owner = GetOwner();
	return owner ? owner->GetScene() : nullptr;
}

UIObject* UINumberSpriteComponent::FindUIObject(const std::string& name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!scene || !uiManager)
	{
		return nullptr;
	}

	auto uiObject = uiManager->FindUIObject(scene->GetName(), name);
	return uiObject ? uiObject.get() : nullptr;
}

void UINumberSpriteComponent::ApplyValue()
{
	if (m_DigitObjectNames.empty())
	{
		return;
	}

	const std::string valueText = std::to_string(m_Value);
	const int digitCount = static_cast<int>(valueText.size());
	const int slotCount = static_cast<int>(m_DigitObjectNames.size());
	const int leadingSlots = max(0, slotCount - digitCount);

	for (int index = 0; index < slotCount; ++index)
	{
		const std::string& name = m_DigitObjectNames[static_cast<size_t>(index)];
		if (name.empty())
		{
			continue;
		}

		auto* target = FindUIObject(name);
		if (!target)
		{
			continue;
		}

		const bool isLeadingSlot = index < leadingSlots;
		if (isLeadingSlot && !m_LeadingZero)
		{
			target->SetIsVisibleFromComponent(false);
			continue;
		}

		const int valueIndex = isLeadingSlot ? 0 : index - leadingSlots;
		const char digitChar = valueText[static_cast<size_t>(valueIndex)];
		const int digitValue = std::clamp(static_cast<int>(digitChar - '0'), 0, 9);

		target->SetIsVisibleFromComponent(true);
		if (auto* image = target->GetComponent<UIImageComponent>())
		{
			const auto& handle = m_DigitTextures[static_cast<size_t>(digitValue)];
			if (handle.IsValid())
			{
				image->SetTextureHandle(handle);
			}
		}
	}
}