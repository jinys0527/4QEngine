#include "UITextComponent.h"

UITextComponent::UITextComponent()
{
}

void UITextComponent::SetText(const std::wstring& text)
{
	m_Text = text;
	m_UseExternalLayout = false;
	RebuildTextResources();
}

void UITextComponent::SetFontSize(float size)
{
	m_FontSize = size;
	m_UseExternalLayout = false;
	RebuildTextResources();
}

void UITextComponent::SetColor(const D2D1_COLOR_F& color)
{
	m_Color = color;
}

void UITextComponent::SetFontName(const std::wstring& fontName)
{
	m_FontName = fontName;
	m_UseExternalLayout = false;
	RebuildTextResources();
}

void UITextComponent::SetTextAlignment(DWRITE_TEXT_ALIGNMENT align)
{
	m_TextAlign = align;
	m_UseExternalLayout = false;
	RebuildTextResources();
}

void UITextComponent::SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT align)
{
	m_ParagraphAlign = align;
	m_UseExternalLayout = false;
	RebuildTextResources();
}

void UITextComponent::SetLayoutSize(float width, float height)
{
	m_LayoutWidth = width;
	m_LayoutHeight = height;
	m_UseExternalLayout = false;
	RebuildTextResources();
}

void UITextComponent::SetTextLayout(Microsoft::WRL::ComPtr<IDWriteTextLayout> layout)
{
	m_TextLayout = layout;
	m_UseExternalLayout = true;
}

void UITextComponent::Serialize(nlohmann::json& j) const
{
}

void UITextComponent::Deserialize(const nlohmann::json& j)
{
}

void UITextComponent::RebuildTextResources()
{
	if (!m_dwriteFactory) return;

	// 텍스트 비어있으면 초기화 생략
	if (m_Text.empty()) return;

	// Create Format
	HRESULT hr = m_dwriteFactory->CreateTextFormat(
		m_FontName.c_str(), nullptr,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		m_FontSize,
		L"ko-kr",
		&m_TextFormat
	);

	if (FAILED(hr)) return; // 폰트 문제 등 실패 처리

	m_TextFormat->SetTextAlignment(m_TextAlign);
	m_TextFormat->SetParagraphAlignment(m_ParagraphAlign);

	// 최소 사이즈 보정 (0일 경우 레이아웃 생성 실패 가능)
	float layoutWidth = m_LayoutWidth > 0 ? m_LayoutWidth : 1.0f;
	float layoutHeight = m_LayoutHeight > 0 ? m_LayoutHeight : 1.0f;

	// Create Layout
	hr = m_dwriteFactory->CreateTextLayout(
		m_Text.c_str(),
		static_cast<UINT32>(m_Text.length()),
		m_TextFormat.Get(),
		layoutWidth,
		layoutHeight,
		&m_TextLayout
	);
	if (FAILED(hr)) return;

	// 줄바꿈 허용
	m_TextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
}

/*
사용 예시
auto text = obj->AddComponent<UITextComponent>();
text->SetDWriteFactory(dwriteFactory);
text->SetText(L"UI 텍스트 예시");
text->SetFontName(L"Segoe UI");
text->SetFontSize(24.0f);
text->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
text->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
text->SetLayoutSize(500.0f, 100.0f);
*/