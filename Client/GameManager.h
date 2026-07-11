#pragma once
 
#include "FSM.h"
#include "EventDispatcher.h"
#include "Event.h"
#include "GameState.h"
#include "GameDataRepository.h"
#include "ShopRoller.h"
#include <string>
#include <vector>


class GameManager : public IEventListener
{
public:
	// Scene 넘어갈 때 가지고 가야할 Data
	struct PlayerPersistentData
	{ // 처음엔 초기셋팅
		bool hasData = false;
		int  weaponCost = 0;
		int  attackRange = 0;
		int  actorId = 1;
		int  money = 0;
		std::vector<std::string> inventoryItemIds;

		int  currentHP = 100;
		int  health = 0;
		int  strength = 0;
		int  agility = 0;
		int  sense = 0;
		int  skill = 0;
		int  equipmentDefenseBonus = 0;
	};
	GameManager();
	~GameManager();

	void SetEventDispatcher(EventDispatcher& eventDispatcher) { m_EventDispatcher = &eventDispatcher; }

	void Reset();

	void OnEvent(EventType type, const void* data);
	void TurnReset();

	void Initial();

	void RequestSceneChange(const std::string& name);

	class GameObject* FindPlayerObject(class Scene* scene) const;
	class GameDataRepository* GetGameDataRepository() const;
	class LogSystem* GetLogSystem() const;
	class RandomMachine* GetRandomMachine() const;
	class DiceSystem* GetDiceSystem() const;
	class ShopRoller* GetShopRoller() const;
	void LoadGameData();
	void InitializePlayer();
	void InitializeFloor();
	void AdvanceFloor();
	void SetPlayerShopState(bool inShop);
	void SetFloodSystemActive(bool active);
	void RefreshGridSystem();
	void DispatchPlayerFSMEvent(const std::string& eventName);
	int ResolvePlayerActorId() const;
	void ResolveEnemyAttack(int actorId = 0);
	bool ResolveEnemyGroupTurn();
	std::vector<int> CollectOwnedItemIndices() const;
private:

	EventDispatcher* m_EventDispatcher = nullptr;

};