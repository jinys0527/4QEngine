#include "SoundUI.h"

SoundUI::SoundUI(SoundManager& soundManager, EventDispatcher& eventDispatcher) : UIObject(eventDispatcher), m_SoundManager(soundManager)
{
// 	m_Master = AddComponent<UISliderComponent>();
// 	m_Master->Start();
	m_BGM = AddComponent<UISliderComponent>();
	m_SFX = AddComponent<UISliderComponent>();
// 	m_UI = AddComponent<UISliderComponent>();
// 	m_UI->Start();
}

void SoundUI::SetSlider()
{
	//m_Master->SetOnValueChanged([this](float value) { m_SoundManager.SetDirty(); m_SoundManager.SetVolume_Master(value); });
	m_BGM->SetOnValueChanged([this](float value) { m_SoundManager.SetDirty(); m_SoundManager.SetVolume_BGM(value); });
	m_SFX->SetOnValueChanged([this](float value) { m_SoundManager.SetDirty(); m_SoundManager.SetVolume_SFX(value); });
	//m_UI->SetOnValueChanged([this](float value) { m_SoundManager.SetDirty(); m_SoundManager.SetVolume_UI(value); });
}
