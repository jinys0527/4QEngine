#pragma once
#include "UISliderComponent.h"
#include "memory"
#include "SoundManager.h"
#include "UIObject.h"

class SoundUI : public UIObject
{
public:
	SoundUI(SoundManager& soundManager, EventDispatcher& eventDispatcher);
	~SoundUI() = default;

/*	UISliderComponent* GetMaster() { return m_Master; }*/
	UISliderComponent* GetBGM() { return m_BGM; }
	UISliderComponent* GetSFX() { return m_SFX; }
/*	UISliderComponent* GetUI() { return m_UI; }*/

	void SetSlider();

private:
	SoundManager& m_SoundManager;

/*	UISliderComponent* m_Master;*/
	UISliderComponent* m_BGM;
	UISliderComponent* m_SFX;
/*	UISliderComponent* m_UI;*/
};

