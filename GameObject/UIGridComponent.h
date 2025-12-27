#pragma once
#include "UIComponent.h"
#include "SimpleMathHelper.h"
#include <memory>
#include <vector>

class UIObject;

class UIGridComponent : public UIComponent
{
	using Vec2F = Math::Vector2F;
public:
	UIGridComponent();
	virtual ~UIGridComponent() = default;

	void SetCellSize(Vec2F size) { m_CellSize = size; }
	void SetPadding(Vec2F padding) { m_Padding = padding; }
	void SetSpacing(Vec2F spacing) { m_Spacing = spacing; }
	void SetRowColumn(int row, int col) { m_Row = row; m_Col = col; }

	void AddItem(std::shared_ptr<UIObject> item);
	std::vector<std::shared_ptr<UIObject>> GetItems() { return m_Items; }
	void ClearItems();

	void UpdateLayout(); // 내부 위치 재계산
private:
	Vec2F m_CellSize;
	Vec2F m_Padding;
	Vec2F m_Spacing;
	int m_Row = 1;
	int m_Col = 1;
	std::vector<std::shared_ptr<UIObject>> m_Items;
};

