#pragma once
#include "Component.h"

// Player Component 는 Player와 관련된 정보와 중요 로직
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

	void SetMoveResource(const int& move)  { m_MoveResource  = move; }
	void SetActResource(const int& act)	   { m_ActResource = act; }
	const int& GetMoveResource()	 const { return m_MoveResource; }
	const int& GetActResource()		 const { return m_ActResource; }


private:

	// 외부지정 가능
	// 이동력, 행동력
	int m_MoveResource = 3;
	int m_ActResource = 6;

	//ReadOnly
	// Grid 기반 좌표
	int m_Q;
	int m_R;

};