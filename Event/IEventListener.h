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

	//UI
	Hovered,
	Pressed,
	Released,
	Moved,
	UIDragged,
	UIDoubleClicked,

	//Collision
	CollisionEnter,
	CollisionStay,
	CollisionExit,
	CollisionTrigger,

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
};
