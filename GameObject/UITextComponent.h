#pragma once
#include "UIComponent.h"
#include <wrl/client.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include "SimpleMathHelper.h"

class UITextComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UITextComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	UITextComponent();
	virtual ~UITextComponent() = default;

	void SetText(const std::wstring& text);
	const std::wstring& GetText() const { return m_Text; }

	void SetFontSize(float size);
	float GetFontSize() const { return m_FontSize; }

	void SetColor(const D2D1_COLOR_F& color);
	const D2D1_COLOR_F& GetColor() const { return m_Color; }

	// TextFormat 설정용
	void SetFontName(const std::wstring& fontName);
	void SetTextAlignment(DWRITE_TEXT_ALIGNMENT align);
	void SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT align);

	// Layout 관련 설정
	void SetLayoutSize(float width, float height);
	void SetTextLayout(Microsoft::WRL::ComPtr<IDWriteTextLayout> layout);

	Microsoft::WRL::ComPtr<IDWriteTextLayout> GetTextLayout()
	{
		return m_TextLayout;
	}

	// DWriteFactory 주입
	void SetDWriteFactory(Microsoft::WRL::ComPtr<IDWriteFactory> dwriteFactory)
	{
		m_dwriteFactory = dwriteFactory;
	}

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void SetPosition(Math::Vector2F pivot) { m_Position = pivot; }
	Math::Vector2F GetPosition() { return m_Position; }

private:
	void RebuildTextResources();

	std::wstring m_Text;
	float m_FontSize = 16.0f;
	std::wstring m_FontName = L"맑은 고딕";
	D2D1_COLOR_F m_Color = D2D1::ColorF(D2D1::ColorF::Black);

	float m_LayoutWidth = 1000.0f;
	float m_LayoutHeight = 1000.0f;

	DWRITE_TEXT_ALIGNMENT m_TextAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
	DWRITE_PARAGRAPH_ALIGNMENT m_ParagraphAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;

	bool m_UseExternalLayout = false;

	Math::Vector2F m_Position = { 0, 0 };

	Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
	Microsoft::WRL::ComPtr<IDWriteTextFormat> m_TextFormat;
	Microsoft::WRL::ComPtr<IDWriteTextLayout> m_TextLayout;
};

REGISTER_COMPONENT(UITextComponent);