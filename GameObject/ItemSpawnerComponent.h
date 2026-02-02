#pragma once
#include "Component.h"
#include <string>	

class GameObject;
class GameDataRepository;

class ItemSpawnerComponent : public Component
{
public:
	static constexpr const char* StaticTypeName = "ItemSpawnerComponent";
	const char* GetTypeName() const override;

	ItemSpawnerComponent() = default;
	virtual ~ItemSpawnerComponent() = default;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SpwanFixedItem();
	void DropItem();
private:
	GameObject* m_SpawnItem = nullptr;

	GameDataRepository* m_GameDataRepository = nullptr;
};

