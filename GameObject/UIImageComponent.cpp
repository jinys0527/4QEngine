#include "UIImageComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(UIImageComponent, UIComponent)
REGISTER_PROPERTY_HANDLE(UIImageComponent, TextureHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, ShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, VertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, PixelShaderHandle)

void UIImageComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UIImageComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}
