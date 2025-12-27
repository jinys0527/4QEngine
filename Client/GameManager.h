#pragma once
 
#include "FSM.h"
#include "EventDispatcher.h"

//enum class DeathReason {
//	Obstacle1,
//	Obstacle2,
//	Obstacle3,
//	BossAttack1,
//	BossAttack2,
//	BOssAttack3
//};

class GameManager : public IEventListener
{
public:
	GameManager(EventDispatcher& eventDispatcher);
	~GameManager();

	void Reset();

	void OnEvent(EventType type, const void* data);

	void Initial();

	int m_playerHp = 5;
	int m_playerReinforcedAttack = 3;
	float m_playerXLoc = 960.0f;

	int m_score = 0;
	int m_hitCount = 0;

	float m_scrollSpeed = 0;

private:

	int m_initialHp = 5;
	int m_initialReinforcedAttack = 3;

	int m_forhitHp = 5;
	int m_prevHp = 5;


	int m_mapTotalLength = 0;// 나중에 게임 정산 하거나 맵 진행도 보여주기 위한 맵 길이 저장
	int m_mapPlayedLength = 0;

	// 플레이어 위치를 저장할 포인터 추가는 안해도 되겠죠?

	FSM m_Fsm; // Fsm 게임 상태인데 나중에 사용할듯

	EventDispatcher& m_EventDispatcher;
};

