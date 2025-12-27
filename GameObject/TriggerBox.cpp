#include "TriggerBox.h"
#include "SpriteRenderer.h"
//#include "AssetManager.h"

void TriggerBox::Start(AssetManager* assetManager)
{
	SetAssetManager(assetManager);

	//m_Sprite->SetTexture(m_AssetManager->LoadTexture(L"TriggerBox", "../Resource/Obstacle/TriggerBox.png"));
	m_Sprite->SetPivotPreset(SpritePivotPreset::BottomCenter, m_Sprite->GetTexture()->GetSize());
	m_Sprite->SetPath("../Resource/Obstacle/TriggerBox.png");
	m_Sprite->SetTextureKey("TriggerBox");
}
