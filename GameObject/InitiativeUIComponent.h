#pragma once
#include "UIComponent.h"
#include "ResourceHandle.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class UIManager;
class Scene;
class UIObject;

class InitiativeUIComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "InitiativeUIComponent";
	const char* GetTypeName() const override;
	~InitiativeUIComponent() override;

	void Start  () override;
	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void		SetEnabled(const bool& enable);
	const bool& GetEnabled() const		{ return m_Enabled;   }

	void		 SetScale(const float& scale);
	const float& GetScale() const { return m_Scale; }
	void				 SetDeadIconTexture(const TextureHandle& handle)   { m_DeadIconTexture = handle;   }
	const TextureHandle& GetDeadIconTexture() const						   { return m_DeadIconTexture;	   }

	void RebuildUI();
	void RemoveUI();

private:
	struct ActorIconInfo
	{
		bool isPlayer	  = false;
		int  displayIndex = 0;
	};

	UIManager* GetUIManager() const;
	Scene*	   GetScene() const;
	void	   RebuildInitiativeOrder();
	std::shared_ptr<UIObject> FindUI(UIManager& uiManager, Scene& scene, const std::string& name) const;
	ActorIconInfo ResolveActorInfo(int actorId) const;
	void ResetIconPools();
	void UpdateIconVisuals(UIObject& iconObject, int actorId, const ActorIconInfo& info) const;
	void UpdateActiveSlotState();
	void SetFrameVisible(bool visible);
	bool IsActorDead(int actorId) const;

	bool  m_Enabled  = false;
	bool  m_InCombat = false;
	float m_Scale = 1.25f;
	int   m_CurrentActorId = 0;
	bool  m_OrderDirty = false;

	std::vector<int> m_InitiativeOrder;
	std::vector<std::shared_ptr<UIObject>> m_PlayerIconPool;
	std::vector<std::shared_ptr<UIObject>> m_EnemyIconPool;
	std::unordered_map<int, std::shared_ptr<UIObject>> m_ActorIcons;
	std::unordered_map<int, ActorIconInfo> m_ActorInfo;
	std::vector<std::string> m_PlayerIconNames{ "InitiativePlayerIcon_1" };
	std::vector<std::string> m_EnemyIconNames{
		"InitiativeEnemyIcon_1",
		"InitiativeEnemyIcon_2",
		"InitiativeEnemyIcon_3",
		"InitiativeEnemyIcon_4",
		"InitiativeEnemyIcon_5",
		"InitiativeEnemyIcon_6",
		"InitiativeEnemyIcon_7",
		"InitiativeEnemyIcon_8",
		"InitiativeEnemyIcon_9",
		"InitiativeEnemyIcon_10"
	};

	TextureHandle m_DeadIconTexture   = TextureHandle::Invalid();

};

