#pragma once
#include "Component.h"
#include "IEventListener.h"
#include "Event.h"
#include <vector>
#include <string>

class PlayerComponent;
class TransformComponent;

class VendingComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "VendingComponent";
	const char* GetTypeName() const override;

	VendingComponent();
	virtual ~VendingComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetDistance(const float& value) { m_Distance = value; }
	const float& GetDistance() const { return m_Distance; }

	bool Clicked(const Events::MouseState* mouseData);

	const int& GetCost() const { return m_Cost; }
	void SetCost(const int& value) { m_Cost = value; }

private:
	
	PlayerComponent* FindPlayerComponent() const;
	bool IsClickedThisVending(const Events::MouseState& mouseData) const; // ray
	bool OpenVendingUI();

	PlayerComponent* m_Player = nullptr;
	float m_Distance = 2.0f;
	int m_Cost = 10;
	int m_DifficultyGroup = 100;

	bool m_HasPreparedCandidates = false;
	std::vector<int> m_PreparedCandidates;
};