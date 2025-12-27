#pragma once
#include "SimpleMathHelper.h"
#include <windows.h>

inline bool IsPointInUIRect(
	const Math::Vector2F& position,   // RectTransform 위치 (pivot 적용 전)
	const Math::Vector2F& size,       // 크기
	const D2D1_POINT_2F& pivot,      // 0~1 pivot
	const POINT& mousePos)             // 검사할 좌표 (스크린 또는 UI 좌표계)
{
	Math::Vector2F topLeft = position;

	return (mousePos.x >= topLeft.x && mousePos.x <= (topLeft.x + size.x) &&
		mousePos.y >= topLeft.y && mousePos.y <= (topLeft.y + size.y));
}