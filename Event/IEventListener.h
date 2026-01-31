#pragma once
 
enum class EventType
{
	//Input
	KeyDown,
	KeyUp,
	MouseLeftClick,
	MouseLeftClickHold,
	MouseLeftClickUp,
	MouseLeftDoubleClick,
	MouseRightClick,
	MouseRightClickHold,
	MouseRightClickUp,
	Dragged, 
	Hovered,

	//UI
	Pressed,
	Released,
	Moved,
	UIHovered,
	UIDragged,
	UIDoubleClicked,

	//Combat
	CombatEnter,
	CombatExit,
	CombatInitiativeBuilt,
	CombatTurnAdvanced,

	//Collision
	CollisionEnter,
	CollisionStay,
	CollisionExit,
	CollisionTrigger,

	//AI
	AITurnEndRequested,
	AIMeleeAttackRequested,
	AIRangedAttackRequested,

	//Turn
	PlayerTurnEndRequested,
	EnemyTurnEndRequested,
	TurnChanged,

	//Scene
	SceneChangeRequested,

	//Player
	PlayerMove,
};

class IEventListener
{
public:
	virtual ~IEventListener() = default;
	virtual void OnEvent(EventType type, const void* data) = 0;
	virtual bool ShouldHandleEvent(EventType type, const void* data)
	{
		(void)type;
		(void)data;
		return true;
	}
	virtual int GetEventPriority() const { return 0; }
};
