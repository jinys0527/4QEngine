#include "pch.h"
#include "Scene.h"
#include "json.hpp"
#include "GameObject.h"
#include "UIObject.h"
#include "GameManager.h"
#include <unordered_set>
#include "CameraComponent.h"
#include "MeshRenderer.h"
#include "LightComponent.h"
#include "TransformComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMeshRenderer.h"
#include "CameraObject.h"

Scene::~Scene()
{
	m_OpaqueObjects.clear();
	m_TransparentObjects.clear();
	//m_Camera = nullptr; // 필요 시
}

// Light, Camera, Fog 등등 GameObject 외의 Scene 구성된 것들 Update
void Scene::StateUpdate(float deltaTime)
{
	// Camera는 BuildFromData를 하면서 자동으로 갱신이 되고있음
	// Light의 경우 LightObject가 생기고 PointLight 같은 애의 위치가 바뀌면 만들수있을것같음?
}

void Scene::Render(RenderData::FrameData& frameData) const
{
	BuildFrameData(frameData);
}

void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject, bool isOpaque)
{
	if (gameObject->m_Name == "Main Camera")
	{
		//SetMainCamera(gameObject);
	}

	if (isOpaque) // true -> Opaque / false -> Transparent
		m_OpaqueObjects[gameObject->m_Name] = std::move(gameObject);
	else
		m_TransparentObjects[gameObject->m_Name] = std::move(gameObject);
}

void Scene::RemoveGameObject(std::shared_ptr<GameObject> gameObject, bool isOpaque)
{
	if (isOpaque)
	{
		auto it = m_OpaqueObjects.find(gameObject->m_Name);
		if (it != m_OpaqueObjects.end())
		{
			gameObject->GetComponent<TransformComponent>()->DetachFromParent();
			m_OpaqueObjects.erase(gameObject->m_Name);
		}
	}
	else
	{
		auto it = m_TransparentObjects.find(gameObject->m_Name);
		if (it != m_TransparentObjects.end())
		{
			gameObject->GetComponent<TransformComponent>()->DetachFromParent();
			m_TransparentObjects.erase(gameObject->m_Name);
		}
	}

}

std::shared_ptr<GameObject> Scene::CreateGameObject(const std::string& name, bool isOpaque)
{
	auto gameObject = std::make_shared<GameObject>(m_EventDispatcher);
	gameObject->SetName(name);
	AddGameObject(gameObject, isOpaque);
	return gameObject;
}


bool Scene::RemoveGameObjectByName(const std::string& name)
{
	auto opaqueIt = m_OpaqueObjects.find(name);
	if (opaqueIt != m_OpaqueObjects.end())
	{
		RemoveGameObject(opaqueIt->second, true);
		return true;
	}

	auto transparentIt = m_TransparentObjects.find(name);
	if (transparentIt != m_TransparentObjects.end())
	{
		RemoveGameObject(transparentIt->second, false);
		return true;
	}

	return false;
}

bool Scene::RenameGameObject(const std::string& currentName, const std::string& newName)
{
	if (currentName == newName || newName.empty())
	{
		return false;
	}

	if (HasGameObjectName(newName))
	{
		return false;
	}

	// 계속 2번 동작 해야됨
	auto opaqueNode = m_OpaqueObjects.extract(currentName);
	if (!opaqueNode.empty())
	{
		opaqueNode.key() = newName;
		opaqueNode.mapped()->SetName(newName);
		m_OpaqueObjects.insert(std::move(opaqueNode));
		return true;
	}

	auto transparentNode = m_TransparentObjects.extract(currentName);
	if (!transparentNode.empty())
	{
		transparentNode.key() = newName;
		transparentNode.mapped()->SetName(newName);
		m_TransparentObjects.insert(std::move(transparentNode));
		return true;
	}

	return false;
}

bool Scene::HasGameObjectName(const std::string& name) const
{
	return m_OpaqueObjects.find(name) != m_OpaqueObjects.end()
		|| m_TransparentObjects.find(name) != m_TransparentObjects.end();
}




void Scene::SetGameCamera(std::shared_ptr<CameraObject> cameraObject)
{
	m_GameCamera = cameraObject;
}

void Scene::SetEditorCamera(std::shared_ptr<CameraObject> cameraObject)
{
	m_EditorCamera = cameraObject;
}


// Opaque, Transparent, UI  ex) ["gameObjects"]["opaque"]
void Scene::Serialize(nlohmann::json& j) const
{	
	//
	// Scene의 Editor 설정 값들
	// Editor 카메라 설정값기억(editor에서만 사용)
	j["editor"] = nlohmann::json::object();
	if (m_EditorCamera)
	{
		nlohmann::json editorCameraJson;
		m_EditorCamera->Serialize(editorCameraJson);
		j["editor"]["EditorCamera"] = editorCameraJson;
	}
	//
	//	GameObject 영역
	//
	j["gameObjects"] = nlohmann::json::object();
	j["gameObjects"]["opaque"] = nlohmann::json::array();
	j["gameObjects"]["transparent"] = nlohmann::json::array();

	// map -> vector 복사
	std::vector<std::pair<std::string, std::shared_ptr<GameObject>>> opaque(
		m_OpaqueObjects.begin(), m_OpaqueObjects.end());
	std::vector<std::pair<std::string, std::shared_ptr<GameObject>>> transparent(
		m_TransparentObjects.begin(), m_TransparentObjects.end());

	//UI는 나중에

	auto sortPred = [](const auto& a, const auto& b) {
		auto extractNameAndNumber = [](const std::string& s) -> std::pair<std::string, int> {
			size_t pos = s.find_first_of("0123456789");
			if (pos != std::string::npos) {
				return { s.substr(0, pos), std::stoi(s.substr(pos)) };
			}
			return { s, -1 };
			};

		auto [nameA, numA] = extractNameAndNumber(a.first);
		auto [nameB, numB] = extractNameAndNumber(b.first);

		if (nameA == nameB) {
			// 같은 종류일 경우 숫자 비교
			return numA < numB;
		}
		// 이름(문자) 기준 비교
		return nameA < nameB;
		};

	// 정렬 (숫자 포함 이름 기준)
	std::sort(opaque.begin(), opaque.end(), sortPred);
	std::sort(transparent.begin(), transparent.end(), sortPred);

	// 정렬된 순서대로 JSON에 저장
	for (const auto& gameObject : opaque)
	{
		nlohmann::json gameObjectJson;
		gameObject.second->Serialize(gameObjectJson);
		j["gameObjects"]["opaque"].push_back(gameObjectJson);
	}

	for (const auto& gameObject : transparent)
	{
		nlohmann::json gameObjectJson;
		gameObject.second->Serialize(gameObjectJson);
		j["gameObjects"]["transparent"].push_back(gameObjectJson);
	}

	//UI
}

template<typename ObjectContainer>
void ProcessWithErase(
	const nlohmann::json& arr,
	ObjectContainer& objContainer,
	EventDispatcher& dispatcher)
{
	std::unordered_set<std::string> names;
	for (const auto& gameObjectJson : arr)
	{
		names.insert(gameObjectJson.at("name").get<std::string>());
	}

	for (const auto& gameObjectJson : arr)
	{
		std::string name = gameObjectJson.at("name").get<std::string>();

		auto it = objContainer.find(name);
		if (it != objContainer.end())
		{	
			it->second->Deserialize(gameObjectJson);
		}
		else
		{
			std::shared_ptr<GameObject> gameObject;
			// 없으면 새로 생성 후 추가
			gameObject = std::make_shared<GameObject>(dispatcher);

			gameObject->Deserialize(gameObjectJson);
			objContainer[name] = std::move(gameObject);
		}
	}

	for (auto it = objContainer.begin(); it != objContainer.end(); )
	{
		if (!names.contains(it->first)) it = objContainer.erase(it);
		else ++it;
	}
}

void Scene::Deserialize(const nlohmann::json& j)
{
	const auto& goRoot = j.at("gameObjects"); //GameObject Root
	const auto& editorRoot = j.at("editor");

	if (editorRoot.contains("EditorCamera")) {
		
	}
	if (goRoot.contains("opaque"))
	{
		ProcessWithErase(goRoot.at("opaque"), m_OpaqueObjects, m_EventDispatcher);
	}
	if (goRoot.contains("transparent"))
	{
		ProcessWithErase(goRoot.at("transparent"), m_TransparentObjects, m_EventDispatcher);
	}
	//UI
}

template <typename ObjectMap>
void AppendFrameDataFromObjects(
	const ObjectMap& objects,
	RenderData::RenderLayer layer,
	RenderData::FrameData& frameData)
{
	for (const auto& [name, gameObject] : objects)
	{
		if (!gameObject)
		{
			continue;
		}

		for (const auto* renderer : gameObject->GetComponents<MeshRenderer>())
		{
			RenderData::RenderItem item{};
			if (renderer && renderer->BuildRenderItem(item))
			{
				frameData.renderItems[layer].push_back(item);
			}
		}

		// object 단위로 미리 계산
		UINT32 paletteOffset = 0;
		UINT32 paletteCount = 0;
		bool hasPalette     = false;

		if (const auto* skeletal = gameObject->GetComponent<SkeletalMeshComponent>())
		{
			const auto& palette = skeletal->GetSkinningPalette();
			if (!palette.empty())
			{
				paletteOffset = (UINT32)frameData.skinningPalettes.size();
				paletteCount  = (UINT32)palette.size();
				frameData.skinningPalettes.insert(frameData.skinningPalettes.end(), palette.begin(), palette.end());
				hasPalette    = true;
			}
		}

		for (const auto* renderer : gameObject->GetComponents<SkeletalMeshRenderer>())
		{
			RenderData::RenderItem item{};
			if (!renderer || !renderer->BuildRenderItem(item))
				continue;

			if (const auto* skeletal = gameObject->GetComponent<SkeletalMeshComponent>())
			{
				item.skeleton = skeletal->GetSkeletonHandle();
			}

			if (hasPalette)
			{
				item.skinningPaletteOffset = paletteOffset;
				item.skinningPaletteCount  = paletteCount;
			}

			frameData.renderItems[layer].push_back(item);
		}


		for (const auto* light : gameObject->GetComponents<LightComponent>())
		{
			if (!light)
			{
				continue;
			}

			RenderData::LightData data{};
			data.type = light->GetType();
			data.posiiton = light->GetPosition();
			data.range = light->GetRange();
			data.diretion = light->GetDirection();
			data.spotAngle = light->GetSpotAngle();
			data.color = light->GetColor();
			data.intensity = light->GetIntensity();
			data.lightViewProj = light->GetLightViewProj();
			data.castShadow = light->GetCastShadow();

			frameData.lights.push_back(data);
		}
	}
}

void Scene::BuildFrameData(RenderData::FrameData& frameData) const
{
	frameData.renderItems.clear();
	frameData.lights.clear();
	frameData.skinningPalettes.clear();

	RenderData::FrameContext& context = frameData.context;
	context = RenderData::FrameContext{};

	// 게임 카메라
	if (m_GameCamera)
	{
		context.gameCamera.view = m_GameCamera->GetViewMatrix();
		context.gameCamera.proj = m_GameCamera->GetProjMatrix();
		const auto viewport = m_GameCamera->GetViewportSize();
		context.gameCamera.width = static_cast<UINT32>(viewport.Width);
		context.gameCamera.height = static_cast<UINT32>(viewport.Height);
		context.gameCamera.cameraPos = m_GameCamera->GetEye();

		const auto view = XMLoadFloat4x4(&context.gameCamera.view);
		const auto proj = XMLoadFloat4x4(&context.gameCamera.proj);
		const auto viewProj = XMMatrixMultiply(view, proj);
		XMStoreFloat4x4(&context.gameCamera.viewProj, viewProj);
	}

	//에디터 nullptr
	if (m_EditorCamera)
	{
		context.editorCamera.view = m_EditorCamera->GetViewMatrix();
		context.editorCamera.proj = m_EditorCamera->GetProjMatrix();
		const auto viewport = m_EditorCamera->GetViewportSize();
		context.editorCamera.width = static_cast<UINT32>(viewport.Width);
		context.editorCamera.height = static_cast<UINT32>(viewport.Height);
		context.editorCamera.cameraPos = m_EditorCamera->GetEye();

		const auto view = XMLoadFloat4x4(&context.editorCamera.view);
		const auto proj = XMLoadFloat4x4(&context.editorCamera.proj);
		const auto viewProj = XMMatrixMultiply(view, proj);
		XMStoreFloat4x4(&context.editorCamera.viewProj, viewProj);
	}

	AppendFrameDataFromObjects(m_OpaqueObjects, RenderData::RenderLayer::OpaqueItems, frameData);

	AppendFrameDataFromObjects(m_TransparentObjects, RenderData::RenderLayer::TransparentItems, frameData);

	//UIManager에서 UI Data 가공예정
}

void Scene::SetGameManager(GameManager* gameManager)
{
	m_GameManager = gameManager;
}

void Scene::SetSceneManager(SceneManager* sceneManager)
{
	m_SceneManager = sceneManager;
}
