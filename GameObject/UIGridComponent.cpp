#include "UIGridComponent.h"
#include "UIImageComponent.h"
#include "RectTransformComponent.h"
#include "Object.h"
#include "UIObject.h"

UIGridComponent::UIGridComponent()
{
}

void UIGridComponent::AddItem(std::shared_ptr<UIObject> item)
{
	m_Items.emplace_back(item);
}

void UIGridComponent::ClearItems()
{
	m_Items.clear();
}

void UIGridComponent::UpdateLayout()
{
	Math::Vector2F pos = m_Owner->GetComponent<RectTransformComponent>()->GetPosition();
	Math::Vector2F size = m_Owner->GetComponent<RectTransformComponent>()->GetSize();
	pos.x = pos.x - 960.0f + (size.x / 2.0f);
	pos.y = pos.y - 540.0f + (size.y / 2.0f);
	for (int i = 0; i < m_Items.size(); ++i)
	{
		int row = i / m_Col;
		int col = i % m_Col;

		float x = m_Padding.x + col * (m_CellSize.x + m_Spacing.x);
		float y = m_Padding.y + row * (m_CellSize.y + m_Spacing.y);

		auto rect = m_Items[i]->GetComponent<RectTransformComponent>();
		rect->SetSize(m_CellSize);
		rect->SetPosition({ pos.x + x, pos.y + y});
	}
}

// auto grid = uiObject->AddComponent<UIGridComponent>();
// grid->SetCellSize({ 64, 64 });
// grid->SetPadding({ 10, 10 });
// grid->SetSpacing({ 5, 5 });
// grid->SetRowColumn(1, 3); // 1За 3ї­
// 
// for (int i = 0; i < 3; ++i)
// {
// 	auto heart = std::make_shared<UIImageComponent>();
// 	heart->SetBitmap(heartTexture);
// 	grid->AddItem(heart);
// }
// 
// grid->UpdateLayout();