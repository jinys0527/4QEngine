#pragma once
#include "GameObject.h"

class AssetManager;
class SpriteRenderer;
class AnimationComponent;

class TriggerBox : public GameObject
{
public:
	TriggerBox(EventDispatcher& eventDispatcher) : GameObject(eventDispatcher)
	{
		m_Sprite = AddComponent<SpriteRenderer>();
	}
	virtual ~TriggerBox() = default;


	void SetAssetManager(AssetManager* assetManager)
	{
		m_AssetManager = assetManager;
	}
	void Start(AssetManager* assetManager);

private:
	AssetManager* m_AssetManager;
	SpriteRenderer* m_Sprite;
};

