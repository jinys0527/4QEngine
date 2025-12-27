#pragma once
#include "SimpleMathHelper.h"

namespace TM
{
	D2D1::Matrix3x2F MakeRenderMatrix(bool bUnityCoords = false, bool bMirror = false, float offsetX = 0.f, float offsetY = 0.f);

	static void MakeMatrixToString(
		const D2D1_MATRIX_3X2_F& matrix,
		__out_ecount(bufSize) wchar_t* buffer,
		size_t bufSize
	)
	{
		swprintf_s(
			buffer,
			bufSize,
			L"%.2f, %.2f\n%.2f, %.2f\n%.2f, %.2f\n",
			matrix._11, matrix._12,
			matrix._21, matrix._22,
			matrix._31, matrix._32
		);
	}

	void DecomposeMatrix3X2(const D2D1::Matrix3x2F& matrix, Math::Vector2F& outTranslation, float& outRotation, Math::Vector2F& outScale);

	D2D1::Matrix3x2F RemovePivot(const D2D1::Matrix3x2F& mat_local, const D2D1_POINT_2F& pivot);

	bool IsPointInRect(const D2D1_POINT_2F& point, const D2D1_RECT_F& rect) noexcept;
}
