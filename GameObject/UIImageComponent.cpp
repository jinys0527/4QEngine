#include "UIImageComponent.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"

REGISTER_UI_COMPONENT(UIImageComponent)
REGISTER_PROPERTY_HANDLE(UIImageComponent, TextureHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, ShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, VertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, PixelShaderHandle)
REGISTER_PROPERTY(UIImageComponent, TintColor)

void UIImageComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UIImageComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

Scene* UIComponent::GetScene() const
{
	auto* owner = GetOwner();
	return owner ? owner->GetScene() : nullptr;
}

UIManager* UIComponent::GetUIManager() const
{
	auto* scene = GetScene();
	if (!scene)
	{
		m_UIManager = nullptr;
		m_UIScene = nullptr;
		return nullptr;
	}

	if (m_UIManager && m_UIScene == scene)
	{
		return m_UIManager;
	}

	auto& services = scene->GetServices();
	if (!services.Has<UIManager>())
	{
		m_UIManager = nullptr;
		m_UIScene = scene;
		return nullptr;
	}

	m_UIManager = &services.Get<UIManager>();
	m_UIScene = scene;
	return m_UIManager;
}