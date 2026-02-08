#include "UIDiceDisplayComponent.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"
#include "UIObject.h"
#include "UIImageComponent.h"
#include <algorithm>

REGISTER_UI_COMPONENT(UIDiceDisplayComponent)
REGISTER_PROPERTY(UIDiceDisplayComponent, Enabled)
REGISTER_PROPERTY(UIDiceDisplayComponent, DiceType)
REGISTER_PROPERTY(UIDiceDisplayComponent, Value)
REGISTER_PROPERTY(UIDiceDisplayComponent, LeadingZero)
REGISTER_PROPERTY(UIDiceDisplayComponent, DiceContext)
REGISTER_PROPERTY(UIDiceDisplayComponent, AutoShow)
REGISTER_PROPERTY(UIDiceDisplayComponent, UseSidesForType)
REGISTER_PROPERTY(UIDiceDisplayComponent, TensDigitObjectName)
REGISTER_PROPERTY(UIDiceDisplayComponent, OnesDigitObjectName)
REGISTER_PROPERTY(UIDiceDisplayComponent, DigitTextures)
REGISTER_PROPERTY(UIDiceDisplayComponent, Layouts)

UIDiceDisplayComponent::~UIDiceDisplayComponent()
{
	if (m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::DiceRolled))
	{
		m_Dispatcher->RemoveListener(EventType::DiceRolled, this);
	}
}

void UIDiceDisplayComponent::Start()
{
	if (auto* scene = GetScene())
	{
		auto& services = scene->GetServices();
		if (services.Has<UIManager>())
		{
			m_UIManager = &services.Get<UIManager>();
		}
	}

	m_Dispatcher  = &GetEventDispatcher();
	m_Dispatcher->AddListener(EventType::DiceRolled, this);
	m_LayoutDirty = true;
	m_ValueDirty  = true;
}

void UIDiceDisplayComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
	(void)deltaTime;

	if (!m_Enabled)
	{
		return;
	}

	if (!m_LayoutDirty && !m_ValueDirty)
	{
		return;
	}

	auto* owner = dynamic_cast<UIObject*>(GetOwner());
	if (!owner)
	{
		return;
	}

	auto* uiManager = GetUIManager();
	if (!uiManager)
	{
		return;
	}

	auto* tens = FindUIObject(m_TensDigitObjectName);
	auto* ones = FindUIObject(m_OnesDigitObjectName);
	const UIDiceLayout* layout = FindLayout();
	if (layout && m_LayoutDirty)
	{
		ApplyLayout(*layout, *owner, tens, ones);
		m_LayoutDirty = false;
	}

	if (m_ValueDirty)
	{
		ApplyValue(tens, ones);
		m_ValueDirty = false;
	}
}

void UIDiceDisplayComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);

	if (!m_Enabled)
	{
		return;
	}

	if (type != EventType::DiceRolled)
	{
		return;
	}

	const auto* payload = static_cast<const Events::DiceRollEvent*>(data);
	if (!payload)
	{
		return;
	}

	if (!m_DiceContext.empty() && payload->context != m_DiceContext)
	{
		return;
	}

	ApplyDiceEvent(*payload);
}

void UIDiceDisplayComponent::SetEnabled(const bool& enabled)
{
	if(m_Enabled == enabled)
	{
		return;
	}

	m_Enabled	  = enabled;
	m_LayoutDirty = true;
	m_ValueDirty  = true;
}

void UIDiceDisplayComponent::SetDiceType(const std::string& type)
{
	if (m_DiceType == type)
	{
		return;
	}

	m_DiceType    = type;
	m_LayoutDirty = true;
}

void UIDiceDisplayComponent::SetValue(const int& value)
{
	const int clamped = std::clamp(value, 0, 24);
	if (m_Value == clamped)
	{
		return;
	}

	m_Value		 = clamped;
	m_ValueDirty = true;
}

void UIDiceDisplayComponent::SetValueFromRollFace(const int& face)
{
	SetValue(face);
}

void UIDiceDisplayComponent::SetValueFromRollFaces(const std::vector<int>& faces, int index)
{
	if (index < 0 || index >= static_cast<int>(faces.size()))
	{
		return;
	}

	SetValue(faces[static_cast<size_t>(index)]);
}

void UIDiceDisplayComponent::SetLeadingZero(const bool& leadingZero)
{
	if (m_LeadingZero == leadingZero)
	{
		return;
	}

	m_LeadingZero = leadingZero;
	m_ValueDirty  = true;
}

void UIDiceDisplayComponent::SetDiceContext(const std::string& context)
{
	if (m_DiceContext == context)
	{
		return;
	}

	m_DiceContext = context;
}

void UIDiceDisplayComponent::SetAutoShow(const bool& autoShow)
{
	m_AutoShow = autoShow;
}

void UIDiceDisplayComponent::SetUseSidesForType(const bool& useSidesForType)
{
	m_UseSidesForType = useSidesForType;
}

void UIDiceDisplayComponent::SetTensDigitObjectName(const std::string& name)
{
	if (m_TensDigitObjectName == name)
	{
		return;
	}

	m_TensDigitObjectName = name;
	m_LayoutDirty		  = true;
	m_ValueDirty		  = true;
}

void UIDiceDisplayComponent::SetOnesDigitObjectName(const std::string& name)
{
	if (m_OnesDigitObjectName == name)
	{
		return;
	}

	m_OnesDigitObjectName = name;
	m_LayoutDirty		  = true;
	m_ValueDirty		  = true;
}

void UIDiceDisplayComponent::SetDigitTextureHandle(int digit, const TextureHandle& handle)
{
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return;
	}

	m_DigitTextures[static_cast<size_t>(digit)] = handle;
	m_ValueDirty = true;
}

const TextureHandle& UIDiceDisplayComponent::GetDigitTextureHandle(int digit) const
{
	static TextureHandle invalid = TextureHandle::Invalid();
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return invalid;
	}
	return m_DigitTextures[static_cast<size_t>(digit)];
}

void UIDiceDisplayComponent::RefreshVisuals()
{
	m_LayoutDirty = true;
	m_ValueDirty  = true;
}

UIManager* UIDiceDisplayComponent::GetUIManager() const
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

Scene* UIDiceDisplayComponent::GetScene() const
{
	auto* owner = GetOwner();
	return owner ? owner->GetScene() : nullptr;
}

UIObject* UIDiceDisplayComponent::FindUIObject(const std::string& name) const
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

void UIDiceDisplayComponent::SetDigitTextures(const std::array<TextureHandle, 10>& textures)
{
	m_DigitTextures = textures;
	m_ValueDirty	= true;
}

void UIDiceDisplayComponent::SetLayouts(std::vector<UIDiceLayout> layouts)
{
	m_Layouts	  = std::move(layouts);
	m_LayoutDirty = true;
}

const UIDiceLayout* UIDiceDisplayComponent::FindLayout() const
{
	if (m_Layouts.empty())
	{
		return nullptr;
	}

	if (m_DiceType.empty())
	{
		return &m_Layouts.front();
	}

	auto it = std::find_if(m_Layouts.begin(), m_Layouts.end(),
		[this](const UIDiceLayout& layout)
		{
			return layout.type == m_DiceType;
		});

	if (it == m_Layouts.end())
	{
		return &m_Layouts.front();
	}

	return &(*it);
}

void UIDiceDisplayComponent::ApplyLayout(const UIDiceLayout& layout, UIObject& owner, UIObject* tens, UIObject* ones)
{
	const UIAnchor topLeftAnchor{ 0.0f, 0.0f };

	if (auto* image = owner.GetComponent<UIImageComponent>())
	{
		if (layout.diceTexture.IsValid())
		{
			image->SetTextureHandle(layout.diceTexture);
		}
	}

	if (tens)
	{
		tens->SetAnchorMin(topLeftAnchor);
		tens->SetAnchorMax(topLeftAnchor);
		tens->SetPivot	  (topLeftAnchor);
		tens->SetBounds	  (layout.tens.bounds);
	}

	if (ones)
	{
		ones->SetAnchorMin(topLeftAnchor);
		ones->SetAnchorMax(topLeftAnchor);
		ones->SetPivot	  (topLeftAnchor);
		ones->SetBounds	  (layout.ones.bounds);
	}
}

void UIDiceDisplayComponent::ApplyValue(UIObject* tens, UIObject* ones)
{
	const int tensDigit = m_Value / 10;
	const int onesDigit = m_Value % 10;

	if (tens)
	{
		if (!m_LeadingZero && tensDigit == 0)
		{
			tens->SetIsVisibleFromComponent(false);
		}
		else
		{
			tens->SetIsVisibleFromComponent(true);
			if (auto* image = tens->GetComponent<UIImageComponent>())
			{
				const auto& handle = m_DigitTextures[static_cast<size_t>(tensDigit)];
				if (handle.IsValid())
				{
					image->SetTextureHandle(handle);
				}
			}
		}
	}

	if (ones)
	{
		ones->SetIsVisibleFromComponent(true);
		if (auto* image = ones->GetComponent<UIImageComponent>())
		{
			const auto& handle = m_DigitTextures[static_cast<size_t>(onesDigit)];
			if (handle.IsValid())
			{
				image->SetTextureHandle(handle);
			}
		}
	}
}

void UIDiceDisplayComponent::ApplyDiceEvent(const Events::DiceRollEvent& payload)
{
	if (m_UseSidesForType && payload.diceSides > 0)
	{
		SetDiceType("D" + std::to_string(payload.diceSides));
	}

	SetValue(payload.value);

	if (m_AutoShow)
	{
		if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
		{
			owner->SetIsVisibleFromComponent(true);
		}
	}
}
