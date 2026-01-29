#pragma once
#include "Windows.h"
#include <string>

namespace Events
{
	struct KeyEvent
	{
		char key;
	};

	struct MouseState
	{
		POINT  pos{ 0, 0 };

		bool   leftPressed { false };
		bool   rightPressed{ false };
		mutable bool   handled{ false };
	};

	struct SceneChangeRequest
	{
		std::string name;
	};

	struct TurnChanged
	{
		int turn = 0;
	};
}