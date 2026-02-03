#pragma once
#include "UIComponent.h"
#include "ResourceHandle.h"
#include <functional>

struct UIButtonStyle
{
	TextureHandle normalTexture = TextureHandle::Invalid();
	TextureHandle hoveredTexture = TextureHandle::Invalid();
	TextureHandle pressedTexture = TextureHandle::Invalid();
	TextureHandle disabledTexture = TextureHandle::Invalid();
	ShaderAssetHandle shaderAsset = ShaderAssetHandle::Invalid();
	VertexShaderHandle vertexShader = VertexShaderHandle::Invalid();
	PixelShaderHandle pixelShader = PixelShaderHandle::Invalid();
};

class UIButtonComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIButtonComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetIsEnabled(const bool& enabled) { m_IsEnabled = enabled; }
	const bool& GetIsEnabled() const { return m_IsEnabled; }

	const bool& GetIsPressed() const { return m_IsPressed; }
	const bool& GetIsHovered() const { return m_IsHovered; }

	void SetStyle(const UIButtonStyle& style) { m_Style = style; }
	const UIButtonStyle& GetStyle() const { return m_Style; }

	void SetNormalTextureHandle(const TextureHandle& handle) { m_Style.normalTexture = handle; }
	const TextureHandle& GetNormalTextureHandle() const { return m_Style.normalTexture; }

	void SetHoveredTextureHandle(const TextureHandle& handle) { m_Style.hoveredTexture = handle; }
	const TextureHandle& GetHoveredTextureHandle() const { return m_Style.hoveredTexture; }

	void SetPressedTextureHandle(const TextureHandle& handle) { m_Style.pressedTexture = handle; }
	const TextureHandle& GetPressedTextureHandle() const { return m_Style.pressedTexture; }

	void SetDisabledTextureHandle(const TextureHandle& handle) { m_Style.disabledTexture = handle; }
	const TextureHandle& GetDisabledTextureHandle() const { return m_Style.disabledTexture; }

	void SetShaderAssetHandle(const ShaderAssetHandle& handle) { m_Style.shaderAsset = handle; }
	const ShaderAssetHandle& GetShaderAssetHandle() const { return m_Style.shaderAsset; }

	void SetVertexShaderHandle(const VertexShaderHandle& handle) { m_Style.vertexShader = handle; }
	const VertexShaderHandle& GetVertexShaderHandle() const { return m_Style.vertexShader; }

	void SetPixelShaderHandle(const PixelShaderHandle& handle) { m_Style.pixelShader = handle; }
	const PixelShaderHandle& GetPixelShaderHandle() const { return m_Style.pixelShader; }

	TextureHandle GetCurrentTextureHandle() const;
	bool HasStyleOverrides() const;

	void SetOnClicked(std::function<void()> callback)
	{
		m_OnClicked = std::move(callback);
	}

	void HandlePressed();
	void HandleReleased();
	void HandleHover(bool isHovered);

private:	
	UIButtonStyle m_Style;
	bool m_IsEnabled = true;
	bool m_IsPressed = false;
	bool m_IsHovered = false;
	std::function<void()> m_OnClicked;
};

