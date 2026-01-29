#pragma once
#include "Component.h"

class PlayerFSMFlowComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "PlayerFSMFlowComponent";
	const char* GetTypeName() const override;

	PlayerFSMFlowComponent();
	~PlayerFSMFlowComponent() override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void	    SetPushCost(const int& cost)		 { m_PushCost = cost;        }
	const int&  GetPushCost() const					 { return m_PushCost;        }
				 									 						     
	void	    SetCombatCost(const int& cost)		 { m_CombatCost = cost;      }
	const int&  GetCombatCost() const				 { return m_CombatCost;      }
				 									        
	void	    SetDoorCost(const int& cost)		 { m_DoorCost = cost;        }
	const int&  GetDoorCost() const					 { return m_DoorCost;        }

	void		SetHasShopNearby(const bool& value)  { m_HasShopNearby = value;  }
	const bool& GetHasShopNearby() const			 { return m_HasShopNearby;   }

	void		SetHasCombatRange(const bool& value) { m_HasCombatRange = value; }
	const bool& GetHasCombatRange() const			 { return m_HasCombatRange;  }
													 
	void		SetHasPushTarget(const bool& value)  { m_HasPushTarget = value;  }
	const bool& GetHasPushTarget() const			 { return m_HasPushTarget;   }
													 							 
	void		SetHasShopSpace(const bool& value)   { m_HasShopSpace = value;   }
	const bool& GetHasShopSpace() const				 { return m_HasShopSpace;	 }
													 							 
	void		SetHasShopMoney(const bool& value)   { m_HasShopMoney = value;   }
	const bool& GetHasShopMoney() const				 { return m_HasShopMoney;	 }

private:
	void HandlePushFlow();
	void HandleCombatFlow();
	void HandleInventoryFlow();
	void HandleShopFlow();
	void HandleDoorFlow();

	int m_PushCost = 1;
	int m_CombatCost = 2;
	int m_DoorCost = 1;

	bool m_HasShopNearby = true;
	bool m_HasCombatRange = true;
	bool m_HasPushTarget = true;
	bool m_HasShopSpace = true;
	bool m_HasShopMoney = true;

	bool m_InventoryOpen = false;
	bool m_ShopOpen = false;
	bool m_DoorOpen = false;
};

