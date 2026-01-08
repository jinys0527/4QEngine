#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include "RenderData.h"
//#include "D2DRenderer.h"
//#include "AssetManager.h"
//#include "SoundAssetManager.h"
#include "SoundManager.h"
#include "UIManager.h"

class NzWndBase;
class GameObject;
class UIObject;
class GameManager;
class SceneManager;
class CameraObject;

class Scene
{
public:
	friend class Editor;

	Scene(EventDispatcher& eventDispatcher, 
		/*AssetManager& assetManager,*/ 
		//SoundAssetManager& soundAssetManager, 
		SoundManager& soundManager, 
		//D2DRenderer& renderer, 
		UIManager& uiManager) 
		: m_EventDispatcher(eventDispatcher), 
		//m_AssetManager(assetManager), 
		//m_SoundAssetManager(soundAssetManager), 
		m_SoundManager(soundManager), 
		//m_Renderer(renderer), 
		m_UIManager(uiManager) {}

	virtual ~Scene();
	virtual void Initialize () = 0;
	virtual void Finalize   () = 0;

	virtual void Enter      () = 0;
	virtual void Leave      () = 0;

	virtual void FixedUpdate() = 0;
	virtual void Update     (float deltaTime) = 0;
	//virtual void Render(std::vector<RenderInfo>& renderInfo, std::vector<UIRenderInfo>& uiRenderInfo, std::vector<UITextInfo>& uiTextInfo) = 0;

	void AddGameObject      (std::shared_ptr<GameObject> gameObject);
	void RemoveGameObject   (std::shared_ptr<GameObject> gameObject);

	// For Editor map 자체 Getter( 수정 불가능 상태 )
	// 수정 해야하는 경우 양 const 제거 ( Add GameObject 는 editor에서 call 하면, Scene의 add object 작동 editor map 직접 수정 X)
	const std::unordered_map<std::string, std::shared_ptr<GameObject>>& GetGameObjects() const { return m_GameObjects; }

	void SetMainCamera(std::shared_ptr<GameObject> gameObject);
	CameraObject* GetMainCamera() { return m_Camera; }

	void Serialize          (nlohmann::json& j) const;
	void Deserialize        (const nlohmann::json& j);
	void BuildFrameData(RenderData::FrameData& frameData) const;

	void SetName            (std::string name) { m_Name = name; }
	std::string GetName     () const     { return m_Name;   }

	void SetGameManager     (GameManager* gameManager);
	void SetSceneManager    (SceneManager* sceneManager);

	void SetIsPause         (bool value) { m_Pause = value; }
	bool GetIsPause         ()           { return m_Pause;  }

protected:
	std::unordered_map<std::string, std::shared_ptr<GameObject>> m_GameObjects;
	EventDispatcher& m_EventDispatcher;
	//D2DRenderer& m_Renderer;
	//AssetManager& m_AssetManager;
	//SoundAssetManager& m_SoundAssetManager;
	SoundManager&   m_SoundManager;
	UIManager&      m_UIManager;
	SceneManager*   m_SceneManager = nullptr;
	GameManager*    m_GameManager = nullptr;
	CameraObject*   m_Camera;
	std::string     m_Name;

	bool m_Pause;
private:
	Scene(const Scene&)            = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&)                 = delete;
	Scene& operator=(Scene&&)      = delete;
};
