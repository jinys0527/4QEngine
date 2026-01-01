#pragma once
#include "Windows.h"

namespace Events
{
	struct KeyEvent
	{
		char key;
	};

	struct MouseState
	{
		POINT  pos{ 0, 0 };

		bool   leftPressed{ false };
		bool   rightPressed{ false };
	};
}