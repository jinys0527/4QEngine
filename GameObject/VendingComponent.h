#pragma once
#include "Component.h"
#include "IEventListener.h"
#include <vector>

class PlayerComponent;
struct ItemDefinition;
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

private:
	
};