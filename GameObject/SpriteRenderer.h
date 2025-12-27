#pragma once
#include "Component.h"
#include "wrl/client.h"
#include "SimpleMathHelper.h"
#include <dwrite.h>
#include <wincodec.h>
#include <d2d1_3.h>
#include <d3d11.h>           // ID3D11Device, ID3D11DeviceContext 등
#include <dxgi1_2.h>         // IDXGISwapChain1 등


class AssetManager;

enum class SpritePivotPreset
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomCenter,
	BottomRight,
	Center,
	HealPivot,
	Phase_2_Arm
};

class SpriteRenderer : public Component
{
	using Vec2F = Math::Vector2F;
public:
	static constexpr const char* StaticTypeName = "SpriteRenderer";
	const char* GetTypeName() const override { return StaticTypeName; }

	SpriteRenderer() : m_FlipX(false), m_FlipY(false) {}
	virtual ~SpriteRenderer() = default;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;


	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void SetTexture(Microsoft::WRL::ComPtr<ID2D1Bitmap1> texture);
	void SetTexture(Microsoft::WRL::ComPtr<ID2D1Bitmap1> texture, D2D1_RECT_F srcRect);
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> GetTexture() const;

	void SetTextureKey(std::string key) { m_TextureKey = key; }
	void SetPath(std::string path) { m_Path = path; }

	void SetAssetManager(AssetManager* assetManager) { m_AssetManager = assetManager; }

	void SetOpacity(float opacity)
	{
		if (opacity < 0.0f) opacity = 0.0f;
		if (opacity > 1.0f) opacity = 1.0f;
		m_Opacity = opacity;
	}

	float GetOpacity() 
	{
		return m_Opacity;
	}

	void SetFlipX(bool flipX);
	bool GetFlipX() const;

	void SetFlipY(bool flipY);
	bool GetFlipY() const;

	void SetLayer(int value);
	int GetLayer() const;

	void SetUseSrcRect(bool useSrcRect) { m_UseSrcRect = useSrcRect; }
	bool GetUseSrcRect() const { return m_UseSrcRect; }

	void SetSrcRect(const D2D1_RECT_F& rect) { m_SrcRect = rect; }
	D2D1_RECT_F GetSrcRect() const { return m_SrcRect; }

	const Vec2F& GetPivot() const
	{
		return m_Pivot;
	}

	void SetPivotPreset(SpritePivotPreset spp, const D2D1_SIZE_F& size);

private:
	float m_Opacity = 1.0f;
	AssetManager* m_AssetManager;
	std::string m_TextureKey;
	std::string m_Path;
	Microsoft::WRL::ComPtr<ID2D1Bitmap1> m_Sprite;
	bool m_FlipX;
	bool m_FlipY;
	Vec2F m_Pivot;
	D2D1_RECT_F m_SrcRect = {};       // 사용할 텍스처 영역
	bool m_UseSrcRect = false;        // 전체 텍스처 쓸지, 일부 영역 쓸지
	int m_Layer = -1;
};

REGISTER_COMPONENT(SpriteRenderer);