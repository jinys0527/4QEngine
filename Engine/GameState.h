#pragma once

// Timer
// 1. 첫번째
enum class Turn {
	PlayerTurn,
	EnemyTurn,
};

//2. Battle Check
enum class Battle {
	NonBattle, // 비전투
	InBattle,  // 전투 중(진입포함)
};


// 상태 단위
// 추후 수정
enum class Phase {
	PlayerMove,
	//---------------------
	ItemPick,
	DoorOpen,
	Attack,

};