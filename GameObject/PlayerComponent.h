#pragma once
#include "Component.h"
#include "GameManager.h"

class GridSystemComponent;

// PlayerComponent 는 Player와 관련된 Data와 중요 로직
// Player의 다른 Component의 중추적인 역할
class PlayerComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "PlayerComponent";
	const char* GetTypeName() const override;

	PlayerComponent();
	virtual ~PlayerComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요



	// Getter Setter
	void SetQR(int q, int r) { m_Q = q, m_R = r; }
	const int& GetQ() const { return m_Q; }
	const int& GetR() const { return m_R; }
	
	void SetPlayerTurnTime(const float& time) { m_PlayerTurnTime = time; }
	const float& GetPlayerTurnTime() const { return m_PlayerTurnTime; }
	const float& GetTurnElapsed() const { return m_TurnElapsed; }
	void SetMoveResource(const int& move)  { m_MoveResource  = move; }
	void SetActResource(const int& act)	   { m_ActResource = act; }

	const int& GetMoveResource()	   const { return m_MoveResource; }
	const int& GetActResource()		   const { return m_ActResource; }
	const int& GetRemainMoveResource() const { return m_RemainMoveResource; }
	const int& GetRemainActResource()  const { return m_RemainActResource; }

	void ResetTurnResources();
	void BeginMove();
	bool CommitMove(int targetQ, int targetR);
	bool ConsumeActResource(int amount);

private:

	// 외부지정 가능
	// 이동력, 행동력
	int m_MoveResource = 3; // 초기설정
	int m_ActResource = 6;  // 초기설정

	//ReadOnly
	// Grid 기반 좌표 // 현재위치
	int m_Q;
	int m_R;

	//내부
	// 남은 값 (턴 변경 시 초기화)
	int m_RemainMoveResource = 0;
	int m_RemainActResource = 0;
	int m_StartQ = 0; 
	int m_StartR = 0; 
	bool m_HasMoveStart = false;
	float m_PlayerTurnTime = 2.0f; // 외부 조정
	float m_TurnElapsed = 0.0f;
	Turn m_LastTurn = Turn::PlayerTurn;
	GameManager* m_GameManager = nullptr;
	GridSystemComponent* m_GridSystem;

};