#pragma once
#include <unordered_set>
#include <windows.h>
#include "Event.h"
#include "RayHelper.h"

inline int GetXFromLParam(LPARAM lp) { return (int)(short)(LOWORD(lp)); }
inline int GetYFromLParam(LPARAM lp) { return (int)(short)(HIWORD(lp)); }

class EventDispatcher;

class InputManager
{
public:
	InputManager() = default;
	~InputManager() = default;

	void SetEventDispatcher(EventDispatcher* eventDispatcher);
	void Update            ();
	void OnKeyDown         (char key);
	void OnKeyUp           (char key);
	bool IsKeyPressed      (char key) const;
	bool OnHandleMessage   (const MSG& msg);
	void HandleMsgMouse    (const MSG& msg);
						   
	void SetViewportRect   (const RECT& rect);
	void ClearViewportRect ();
	bool TryGetMouseNDC    (DirectX::XMFLOAT2& outNdc) const;
	bool BuildPickRay      (const DirectX::XMFLOAT4X4& view, 
						    const DirectX::XMFLOAT4X4& proj,
						    DirectX::XMFLOAT3& outOrigin,
						    DirectX::XMFLOAT3& outDirection) const;

	bool BuildPickRay      (const DirectX::XMFLOAT4X4& view,
						    const DirectX::XMFLOAT4X4& proj,
						    const Events::MouseState& mouseState,
						    DirectX::XMFLOAT3& outOrigin,
						    DirectX::XMFLOAT3& outDirection) const;
	
	// 추가: Ray 자체를 반환하는 오버로드
	bool BuildPickRay		(const DirectX::XMFLOAT4X4& view,
							 const DirectX::XMFLOAT4X4& proj,
							 Ray& outRay) const;

	bool BuildPickRay		(const DirectX::XMFLOAT4X4& view,
							 const DirectX::XMFLOAT4X4& proj,
							 const Events::MouseState& mouseState,
							 Ray& outRay) const;

	bool IsPointInViewport(const POINT& point) const;

	void SetEnabled(bool enabled);
	bool IsEnabled () const       { return m_Enabled;    }

private:
	void ResetState();

	std::unordered_set<char> m_KeysDown;			// 현재 눌림
	std::unordered_set<char> m_KeysDownPrev;		// 이전 프레임 눌림

	Events::MouseState       m_Mouse;
	Events::MouseState       m_MousePrev;

	// 더블 클릭 판정 파라미터
	int		  m_DoubleClickThereshlodsMs = 250;
	int		  m_DoubleClickMaxDistance   = 4;

	// 싱글클릭 지연 처리용(pending)
	bool             m_PendingLeftClick = false;
	ULONGLONG        m_PendingLeftClickTime = 0;
	POINT            m_PendingLeftClickPos{ 0, 0 };
	Events::MouseState m_PendingLeftClickMouse; // 싱글 확정 시 보낼 상태 스냅샷

	RECT      m_ViewportRect{ 0, 0, 0, 0 };
	bool      m_HasViewportRect = false;
	bool      m_Enabled = true;

	EventDispatcher*       m_EventDispatcher;				// 참조 보관
};

