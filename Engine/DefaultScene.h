#pragma once

#include "Scene.h"

class DefaultScene : public Scene {

public:
	DefaultScene(EventDispatcher& eventDispatcher, SoundManager& soundManager,
			   UIManager& uiManager) :
				Scene(eventDispatcher, soundManager, uiManager) {}

	virtual ~DefaultScene() = default;

	void Initialize() override;
	void Finalize() override;
	void Enter() override;
	void Leave() override;
	void FixedUpdate() override;
	void Update(float deltaTime) override;
};