#pragma once

#include "Scene.h"

class DefaultScene : public Scene {

public:
	DefaultScene(ServiceRegistry& serviceRegistry) : Scene(serviceRegistry) {}

	virtual ~DefaultScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Leave() override;
	void FixedUpdate() override;
	void Update(float deltaTime) override;
	void StateUpdate(float deltaTime) override;
};