#pragma once
 
enum class EventType
{
	//Input
	KeyDown,
	KeyUp,
	MouseLeftClick,
	MouseLeftClickHold,
	MouseLeftClickUp,
	MouseRightClick,
	MouseRightClickHold,
	MouseRightClickUp,

	//UI
	Hovered,
	Pressed,
	Released,
	Moved,
	Dragged,

	//Collision
	CollisionEnter,
	CollisionStay,
	CollisionExit,
	CollisionTrigger,

	//Game
	OnPlayerCollisonOccur, // 플레이어 충돌 플레이어 피격 전 무적 판정 등 체크
	OnPlayerReinforcedCollisionOccur, // 플레이어 충돌 플레이어 피격 전 무적 판정 등 체크
	OnPlayerHpChanged, // 플레이어 진짜 피격
	OnPlayerReinforcedBulletChanged,
	OnPlayerDeath, // 플레이어 죽음
	OnScoreChange,
	OnBossScene,
	OnPlayerAttack,
	Quit,

	OnBossAttacked,
	OnLoadedScene
};

class IEventListener
{
public:
	virtual ~IEventListener() = default;
	virtual void OnEvent(EventType type, const void* data) = 0;
};
