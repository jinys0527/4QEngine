#include "SpriteRenderer.h"
#include <filesystem>
//#include "AssetManager.h"

void SpriteRenderer::SetPivotPreset(SpritePivotPreset spp, const D2D1_SIZE_F& size)
{
	switch (spp)
	{
	case SpritePivotPreset::TopLeft:
		m_Pivot = { 0.0f, 0.0f };
		break;
	case SpritePivotPreset::TopRight:
		m_Pivot = { size.width, 0.0f };
		break;
	case SpritePivotPreset::Center:
		m_Pivot = { size.width * 0.5f, size.height * 0.5f };
		break;
	case SpritePivotPreset::BottomLeft:
		m_Pivot = { 0.0f, size.height };
		break;
	case SpritePivotPreset::BottomCenter:
		m_Pivot = { size.width * 0.5f, size.height };
		break;
	case SpritePivotPreset::BottomRight:
		m_Pivot = { size.width, size.height };
		break;
	case SpritePivotPreset::HealPivot:
		m_Pivot = { size.width * 0.5f, size.height * 1.5f };
		break;
	case SpritePivotPreset::Phase_2_Arm:
		m_Pivot = { size.width * 0.5f - 250.f, size.height - 450.f };
		break;
	}
}

void SpriteRenderer::Update(float deltaTime)
{
}

void SpriteRenderer::OnEvent(EventType type, const void* data)
{
}

void SpriteRenderer::Serialize(nlohmann::json& j) const
{
	j["textureKey"] = m_TextureKey;
	j["path"] = m_Path;
	j["filpX"] = m_FlipX;
	j["filpY"] = m_FlipY;
	j["opacity"] = m_Opacity;
}

void SpriteRenderer::Deserialize(const nlohmann::json& j)
{
	m_TextureKey = j["textureKey"].get<std::string>();
	m_Path = j["path"].get<std::string>();
	m_FlipX = j["filpX"].get<bool>();
	m_FlipY = j["filpY"].get<bool>();
	m_Opacity = j["opacity"].get<float>();

	std::wstring key(m_TextureKey.begin(), m_TextureKey.end());

	//m_Sprite = m_AssetManager->LoadTexture(key, m_Path);
}

void SpriteRenderer::SetTexture(Microsoft::WRL::ComPtr<ID2D1Bitmap1> texture)
{
	m_Sprite = texture;
	m_UseSrcRect = false;
}

void SpriteRenderer::SetTexture(Microsoft::WRL::ComPtr<ID2D1Bitmap1> texture, D2D1_RECT_F srcRect)
{
	m_Sprite = texture;
	m_SrcRect = srcRect;
	m_UseSrcRect = true;
}

Microsoft::WRL::ComPtr<ID2D1Bitmap1> SpriteRenderer::GetTexture() const
{
	return m_Sprite;
}

void SpriteRenderer::SetFlipX(bool flipX)
{
	m_FlipX = flipX;
}

bool SpriteRenderer::GetFlipX() const
{
	return m_FlipX;
}

void SpriteRenderer::SetFlipY(bool flipY)
{
	m_FlipY = flipY;
}

bool SpriteRenderer::GetFlipY() const
{
	return m_FlipY;
}

void SpriteRenderer::SetLayer(int value)
{
	m_Layer = value;
}

int SpriteRenderer::GetLayer() const
{
	return m_Layer;
}


