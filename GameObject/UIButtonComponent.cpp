#include "UIButtonComponent.h"
#include "ReflectionMacro.h"
#include "UIFSMComponent.h"

REGISTER_UI_COMPONENT(UIButtonComponent)
REGISTER_PROPERTY(UIButtonComponent, IsEnabled)
REGISTER_PROPERTY_READONLY(UIButtonComponent, IsPressed)
REGISTER_PROPERTY_READONLY(UIButtonComponent, IsHovered)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, NormalTextureHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, HoveredTextureHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, PressedTextureHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, DisabledTextureHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, ShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, VertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, PixelShaderHandle)

void UIButtonComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UIButtonComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

TextureHandle UIButtonComponent::GetCurrentTextureHandle() const
{
	if (!m_IsEnabled && m_Style.disabledTexture.IsValid())
	{
		return m_Style.disabledTexture;
	}
	if (m_IsPressed && m_Style.pressedTexture.IsValid())
	{
		return m_Style.pressedTexture;
	}
	if (m_IsHovered && m_Style.hoveredTexture.IsValid())
	{
		return m_Style.hoveredTexture;
	}
	return m_Style.normalTexture;
}

bool UIButtonComponent::HasStyleOverrides() const
{
	return m_Style.normalTexture.IsValid()
		|| m_Style.hoveredTexture.IsValid()
		|| m_Style.pressedTexture.IsValid()
		|| m_Style.disabledTexture.IsValid()
		|| m_Style.shaderAsset.IsValid()
		|| m_Style.vertexShader.IsValid()
		|| m_Style.pixelShader.IsValid();
}

void UIButtonComponent::HandlePressed()
{
	if (!m_IsEnabled)
		return;
	m_IsPressed = true;
}

void UIButtonComponent::HandleReleased()
{
	if (!m_IsEnabled)
		return;

	if (m_IsPressed && m_OnClicked)
	{
		m_OnClicked();
	}
	if (m_IsPressed)
	{
		if (auto* owner = GetOwner())
		{
			if (auto* fsm = owner->GetComponent<UIFSMComponent>())
			{
				fsm->TriggerEventByName("UI_Clicked");
			}
		}
	}

	m_IsPressed = false;
}


void UIButtonComponent::HandleHover(bool isHovered)
{
	if (!m_IsEnabled)
		return;

	m_IsHovered = isHovered;
}
