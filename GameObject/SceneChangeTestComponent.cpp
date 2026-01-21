#include "SceneChangeTestComponent.h"
#include "ReflectionMacro.h"
#include "ServiceRegistry.h"
#include "Object.h"
#include "Event.h"
#include "Scene.h"


REGISTER_COMPONENT(SceneChangeTestComponent)

SceneChangeTestComponent::SceneChangeTestComponent()
{

}

SceneChangeTestComponent::~SceneChangeTestComponent()
{
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);
}

void SceneChangeTestComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::KeyDown, this); // 이벤트 추가
	GetEventDispatcher().AddListener(EventType::KeyUp, this);
}

void SceneChangeTestComponent::Update(float deltaTime)
{
	if (!GetOwner()->GetScene())
		return;
	if (m_IsF12) {

		SceneChange();
	}
}

void SceneChangeTestComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::KeyUp) {
		auto keyData = static_cast<const Events::KeyEvent*>(data);
		if (!keyData) return;
		bool isDown = (type == EventType::KeyDown);
		switch (keyData->key) {
		
		case VK_F12: m_IsF12 = isDown; break;
		default:break;

		}
	}
}

void SceneChangeTestComponent::SceneChange()
{
	Scene* scene = GetOwner()->GetScene();
	if (!scene)
		return;

	auto& services = scene->GetServices();
	auto& gameManager = services.Get<GameManager>();

	//gameManager.RequestSceneChange("Game Test2");
}
