#pragma once

#include "FSM.h"
#include "EventDispatcher.h"
#include "Event.h"
#include "GameState.h"
#include <string>
#include <vector>

class GameManager : public IEventListener
{
public:
	struct PlayerPersistentData
	{
		bool hasData = false;
		int  weaponCost = 0;
		int  attackRange = 0;
		int  actorId = 1;
		int  money = 0;
		std::vector<std::string> inventoryItemIds;

		int  currentHP = 0;
		int  health = 0;
		int  strength = 0;
		int  agility = 0;
		int  sense = 0;
		int  skill = 0;
		int  equipmentDefenseBonus = 0;
	};
	GameManager();
	~GameManager();

	void SetEventDispatcher(EventDispatcher& eventDispatcher);
	void ClearEventDispatcher();

	void Reset();

	void OnEvent(EventType type, const void* data);
	void TurnReset();

	void Initial();

	void RequestSceneChange(const std::string& name);

	Turn GetTurn() const { return m_Turn; }
	void SetTurn(Turn turn);

	void CapturePlayerData(class Scene* scene);
	void ApplyPlayerData(class Scene* scene);

private:
	void RegisterEventListeners();
	void UnregisterEventListeners();
	void DispatchTurnChanged();
	void SyncTurnFromActorId(int actorId);

	class GameObject* FindPlayerObject(class Scene* scene) const;
private:

	EventDispatcher* m_EventDispatcher = nullptr;
	Turn m_Turn;
	Battle m_BattleCheck;
	Phase m_Phase; 

	PlayerPersistentData m_PlayerData;
};