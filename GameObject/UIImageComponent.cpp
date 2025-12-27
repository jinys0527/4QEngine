#include "UIImageComponent.h"

void UIImageComponent::SetBitmap(Microsoft::WRL::ComPtr<ID2D1Bitmap1> bitmap)
{
	m_Bitmap = bitmap;
}

void UIImageComponent::SetUV(const D2D1_RECT_F& sourceRect)
{
	m_SourceRect = sourceRect;
}

void UIImageComponent::Render(D2DRenderer* renderer)
{

}

void UIImageComponent::SetPivotPreset(ImagePivotPreset spp, const D2D1_SIZE_F& size)
{
	switch (spp)
	{
	case ImagePivotPreset::TopLeft:
		m_Pivot = { 0.0f, 0.0f };
		break;
	case ImagePivotPreset::TopRight:
		m_Pivot = { size.width, 0.0f };
		break;
	case ImagePivotPreset::Center:
		m_Pivot = { size.width * 0.5f, size.height * 0.5f };
		break;
	case ImagePivotPreset::BottomLeft:
		m_Pivot = { 0.0f, size.height };
		break;
	case ImagePivotPreset::BottomRight:
		m_Pivot = { size.width, size.height };
		break;
	}

}

Microsoft::WRL::ComPtr<ID2D1Bitmap1> UIImageComponent::GetTexture() const
{
	return m_Bitmap;
}

void UIImageComponent::Serialize(nlohmann::json& j) const
{
}

void UIImageComponent::Deserialize(const nlohmann::json& j)
{
}
