#pragma once
#include "UIComponent.h"
#include "ResourceHandle.h"

class UIImageComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIImageComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetTextureHandle(const TextureHandle& handle) { m_Texture = handle; }
	const TextureHandle& GetTextureHandle() const      { return m_Texture;   }

	void SetShaderAssetHandle(const ShaderAssetHandle& handle) { m_ShaderAssetHandle = handle; }
	const ShaderAssetHandle& GetShaderAssetHandle() const	   { return m_ShaderAssetHandle;   }

	void SetVertexShaderHandle(const VertexShaderHandle& handle) { m_VertexShaderHandle = handle; }
	const VertexShaderHandle& GetVertexShaderHandle() const      { return m_VertexShaderHandle;   }

	void SetPixelShaderHandle(const PixelShaderHandle& handle) { m_PixelShaderHandle = handle; }
	const PixelShaderHandle& GetPixelShaderHandle() const      { return m_PixelShaderHandle;   }
private:
	TextureHandle m_Texture = TextureHandle::Invalid();
	ShaderAssetHandle m_ShaderAssetHandle = ShaderAssetHandle::Invalid();
	VertexShaderHandle m_VertexShaderHandle = VertexShaderHandle::Invalid();
	PixelShaderHandle m_PixelShaderHandle = PixelShaderHandle::Invalid();
};

