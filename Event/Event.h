#pragma once
#include "Windows.h"
#include <string>
#include <vector>

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

	struct ActorEvent
	{
		int actorId = 0;
	};

	struct DiceRollEvent
	{
		int value	  = 0;
		int diceCount = 0;
		int diceSides = 0;
		int bonus	  = 0;
		std::string context;
		bool isTotal = false;
		std::vector<int> faces;
	};
}