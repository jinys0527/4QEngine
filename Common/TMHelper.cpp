#include "pch.h"
#include "TMHelper.h"

namespace TM
{
	D2D1::Matrix3x2F MakeRenderMatrix(bool bUnityCoords, bool bMirror, float offsetX, float offsetY)
	{
		float scaleY = bUnityCoords ? -1.0f : 1.0f;
		float scaleX = bMirror ? -1.0f : 1.0f;

		offsetX = bMirror ? offsetX : -offsetX;
		offsetY = bUnityCoords ? offsetY : -offsetY;

		return D2D1::Matrix3x2F::Scale(scaleX, scaleY) * D2D1::Matrix3x2F::Translation(offsetX, offsetY);
	}

	void DecomposeMatrix3X2(const D2D1::Matrix3x2F& matrix, Math::Vector2F& outTranslation, float& outRotation, Math::Vector2F& outScale)
	{
		outTranslation.x = matrix._31;
		outTranslation.y = matrix._32;

		outScale.x = sqrtf(matrix._11 * matrix._11 + matrix._12 * matrix._12);
		outScale.y = sqrtf(matrix._21 * matrix._21 + matrix._22 * matrix._22);

		const float radians = static_cast<float>(atan2(matrix._12, matrix._11));

		outRotation = Math::RadToDeg(radians);
	}

	D2D1::Matrix3x2F RemovePivot(const D2D1::Matrix3x2F& mat_local, const D2D1_POINT_2F& pivot)
	{
		auto pivot0 = D2D1::Matrix3x2F::Translation(-pivot.x, -pivot.y);

		auto pivot1 = D2D1::Matrix3x2F::Translation(pivot.x, pivot.y);

		return pivot1 * mat_local * pivot0;
	}

	bool IsPointInRect(const D2D1_POINT_2F& point, const D2D1_RECT_F& rect) noexcept
	{
		float left = std::min(rect.left, rect.right);
		float right = std::max(rect.left, rect.right);
		float top = std::min(rect.top, rect.bottom);
		float bottom = std::max(rect.top, rect.bottom);

		return (point.x >= left && point.x <= right) && (point.y >= top && point.y <= bottom);
	}
}
