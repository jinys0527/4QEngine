#include "VendingComponent.h"
#include "Object.h"
#include "Scene.h"
#include "GameObject.h"
#include "ReflectionMacro.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "PlayerShopFSMComponent.h"
#include "ItemSpawnerComponent.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "CameraObject.h"
#include "ServiceRegistry.h"
#include "InputManager.h"
#include "RayHelper.h"
#include "GameDataRepository.h"
#include "AssetLoader.h"
#include "UIImageComponent.h"
#include "UIObject.h"
#include "UIManager.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "UINumberSpriteComponent.h"

REGISTER_COMPONENT(VendingComponent)
REGISTER_PROPERTY(VendingComponent, Distance)

namespace
{
	std::string NormalizePath(std::string value)
	{
		std::replace(value.begin(), value.end(), '\\', '/');
		return value;
	}

	bool EndsWithPath(const std::string& value, const std::string& suffix)
	{
		if (suffix.empty() || value.size() < suffix.size())
		{
			return false;
		}

		return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
	}

	TextureHandle ResolveTextureByPath(AssetLoader& assetLoader, const std::string& path)
	{
		if (path.empty())
		{
			return TextureHandle::Invalid();
		}

		const std::string normalizedPath = NormalizePath(path);
		const auto& keyToHandle = assetLoader.GetTextures().GetKeyToHandle();
		auto direct = keyToHandle.find(normalizedPath);
		if (direct != keyToHandle.end())
		{
			return direct->second;
		}

		std::string trimmedPath = normalizedPath;
		while (trimmedPath.rfind("../", 0) == 0)
		{
			trimmedPath.erase(0, 3);
			auto trimmed = keyToHandle.find(trimmedPath);
			if (trimmed != keyToHandle.end())
			{
				return trimmed->second;
			}
		}

		const size_t filenameStart = normalizedPath.find_last_of('/');
		const std::string filename = (filenameStart == std::string::npos)
			? normalizedPath
			: normalizedPath.substr(filenameStart + 1);
		if (filename.empty())
		{
			return TextureHandle::Invalid();
		}

		for (const auto& [key, handle] : keyToHandle)
		{
			const std::string normalizedKey = NormalizePath(key);
			if (normalizedKey == normalizedPath || EndsWithPath(normalizedKey, normalizedPath) || EndsWithPath(normalizedKey, filename))
			{
				return handle;
			}
		}

		return TextureHandle::Invalid();
	}


	UIImageComponent* FindImageComponentOrFirstChildImage(UIObject& object)
	{
		return object.GetComponent<UIImageComponent>();
	}

	void UpdateVendingSlotInfoUI(Scene& scene, const std::vector<int>& itemIds)
	{
		auto& services = scene.GetServices();
		if (!services.Has<UIManager>() || !services.Has<AssetLoader>() || !services.Has<GameDataRepository>())
		{
			return;
		}

		auto& uiManager = services.Get<UIManager>();
		auto& loader = services.Get<AssetLoader>();
		auto& repository = services.Get<GameDataRepository>();
		const std::string& sceneName = scene.GetName();

		for (int slot = 0; slot < 6; ++slot)
		{
			const int itemId = slot < static_cast<int>(itemIds.size()) ? itemIds[slot] : -1;
			const ItemDefinition* definition = itemId > 0 ? repository.GetItem(itemId) : nullptr;
			TextureHandle infoTexture = TextureHandle::Invalid();
			if (definition && !definition->infoPath.empty())
			{
				infoTexture = ResolveTextureByPath(loader, definition->infoPath);
			}

			const std::string panelName = "VendingSlot" + std::to_string(slot + 1) + "Info";
			auto panel = uiManager.FindUIObject(sceneName, panelName);
			if (!panel)
			{
				continue;
			}
			auto* panelObject = dynamic_cast<UIObject*>(panel.get());
			if (!panelObject)
			{
				continue;
			}
			auto* image = FindImageComponentOrFirstChildImage(*panelObject);
			if (!image)
			{
				continue;
			}
			if (infoTexture.IsValid())
			{
				image->SetTextureHandle(infoTexture);
			}
		}
	}

	void UpdateVendingSlotInfoUI(Scene& scene, const std::vector<int>& itemIds, const std::vector<int>& itemCounts)
	{
		auto& services = scene.GetServices();
		if (!services.Has<UIManager>() || !services.Has<AssetLoader>() || !services.Has<GameDataRepository>())
		{
			return;
		}

		auto& uiManager = services.Get<UIManager>();
		auto& loader = services.Get<AssetLoader>();
		auto& repository = services.Get<GameDataRepository>();
		const std::string& sceneName = scene.GetName();

		for (int slot = 0; slot < 6; ++slot)
		{
			const int itemId = slot < static_cast<int>(itemIds.size()) ? itemIds[slot] : -1;
			const ItemDefinition* definition = itemId > 0 ? repository.GetItem(itemId) : nullptr;
			TextureHandle infoTexture = TextureHandle::Invalid();
			if (definition && !definition->infoPath.empty())
			{
				infoTexture = ResolveTextureByPath(loader, definition->infoPath);
			}

			const std::string panelName = "VendingSlot" + std::to_string(slot + 1) + "Info";
			auto panel = uiManager.FindUIObject(sceneName, panelName);
			if (!panel)
			{
				continue;
			}
			auto* panelObject = dynamic_cast<UIObject*>(panel.get());
			if (!panelObject)
			{
				continue;
			}
			auto* image = FindImageComponentOrFirstChildImage(*panelObject);
			if (!image)
			{
				continue;
			}
			if (infoTexture.IsValid())
			{
				image->SetTextureHandle(infoTexture);
			}

			const int itemCount = slot < static_cast<int>(itemCounts.size()) ? itemCounts[slot] : 0;
			const std::array<std::string, 3> countObjectNames =
			{
				"VendingSlot" + std::to_string(slot + 1) + "Count",
				"VendingSlot" + std::to_string(slot + 1) + "Number",
				"VendingSlot" + std::to_string(slot + 1) + "Amount"
			};

			for (const auto& countName : countObjectNames)
			{
				auto countObject = uiManager.FindUIObject(sceneName, countName);
				if (!countObject)
				{
					continue;
				}

				auto* uiCountObject = dynamic_cast<UIObject*>(countObject.get());
				if (!uiCountObject)
				{
					continue;
				}

				if (auto* number = uiCountObject->GetComponent<UINumberSpriteComponent>())
				{
					number->SetValue(itemCount);
					break;
				}
			}
		}
	}

	//거리 계산용
	float DistanceSquared(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;

		return dx * dx + dz * dz;
	}
}

VendingComponent::VendingComponent()
{
}

VendingComponent::~VendingComponent()
{
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::VendingOfferUpdated, this);
}

void VendingComponent::Start()
{
	m_Player = FindPlayerComponent();
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::VendingOfferUpdated, this);
}

void VendingComponent::Update(float deltaTime)
{
}


void VendingComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::VendingOfferUpdated && data)
	{
		const auto* payload = static_cast<const Events::VendingOfferUpdatedEvent*>(data);
		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		if (!payload || !owner || !scene)
		{
			return;
		}

		if (!payload->vendingObjectName.empty() && payload->vendingObjectName != owner->GetName())
		{
			return;
		}

		UpdateVendingSlotInfoUI(*scene, payload->itemIds, payload->itemCounts);
		return;
	}

	if (type != EventType::MouseLeftClick)
	{
		return;
	}

	auto* mouseData = static_cast<const Events::MouseState*>(data);
	if (!mouseData || mouseData->handled)
	{
		return;
	}

	if (Clicked(mouseData))
	{
		mouseData->handled = true;
	}
}


// bool Click 되었는지 // 플레이어와 거리조건 까지 고려된것

bool VendingComponent::Clicked(const Events::MouseState* mouseData)
{
	auto* owner = GetOwner();
	if (!owner || !mouseData)
	{
		return false;
	}

	if (!m_Player)
	{
		m_Player = FindPlayerComponent();
	}

	if (!IsClickedThisVending(*mouseData))
	{
		return false;
	}

	auto* vendingTransform = owner->GetComponent<TransformComponent>();
	if (!m_Player || !vendingTransform)
	{
		return false;
	}

	auto* playerObject = m_Player->GetOwner();
	auto* playerTransform = playerObject ? playerObject->GetComponent<TransformComponent>() : nullptr;
	if (!playerTransform)
	{
		return false;
	}

	const XMFLOAT3 vendingPos = vendingTransform->GetWorldPos();
	const XMFLOAT3 playerPos = playerTransform->GetWorldPos();
	const float distanceSq = DistanceSquared(vendingPos, playerPos);
	const float minDistanceSq = m_Distance * m_Distance;

	if (distanceSq > minDistanceSq)
	{
		return false;
	}

	std::cout << "[Vending] Interacted. distance="
		<< std::sqrt(distanceSq) << " threshold=" << m_Distance << std::endl;
	return OpenVendingUI();
}

PlayerComponent* VendingComponent::FindPlayerComponent() const
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return nullptr;
	}

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		(void)name;
		if (!object)
		{
			continue;
		}

		if (auto* player = object->GetComponent<PlayerComponent>())
		{
			return player;
		}
	}

	return nullptr;
}

bool VendingComponent::IsClickedThisVending(const Events::MouseState& mouseData) const
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	auto* collider = owner->GetComponent<BoxColliderComponent>();
	auto* scene = owner->GetScene();
	if (!collider || !scene)
	{
		return false;
	}

	auto camera = scene->GetGameCamera();
	if (!camera)
	{
		return false;
	}

	auto& services = scene->GetServices();
	if (!services.Has<InputManager>())
	{
		return false;
	}

	auto& input = services.Get<InputManager>();
	Ray pickRay{};
	if (!input.IsPointInViewport(mouseData.pos))
	{
		return false;
	}

	if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), mouseData, pickRay))
	{
		return false;
	}

	float hitT = 0.0f;
	return collider->IntersectsRay(pickRay.m_Pos, pickRay.m_Dir, hitT);
}

bool VendingComponent::OpenVendingUI()
{
	if (!m_Player)
	{
		return false;
	}

	auto* playerObject = m_Player->GetOwner();
	auto* shopFSM = playerObject ? playerObject->GetComponent<PlayerShopFSMComponent>() : nullptr;
	if (!shopFSM)
	{
		return false;
	}

	auto* spawner = GetOwner() ? GetOwner()->GetComponent<ItemSpawnerComponent>() : nullptr;
	std::vector<int> vendingCandidates;
	if (spawner)
	{
		vendingCandidates = spawner->PrepareVendingRandomCandidates();
	}
	shopFSM->ConfigureVendingOffer(m_Cost, vendingCandidates, spawner);
	if (auto* owner = GetOwner())
	{
		if (auto* scene = owner->GetScene())
		{
			Events::VendingOfferUpdatedEvent payload;
			payload.vendingObjectName = owner->GetName();
			payload.itemIds = vendingCandidates;
			payload.itemCounts.reserve(vendingCandidates.size());
			if (spawner)
			{
				for (const int itemId : vendingCandidates)
				{
					payload.itemCounts.push_back(spawner->GetRemainingDropQuantity(itemId));
				}
			}
			else
			{
				payload.itemCounts.assign(vendingCandidates.size(), 0);
			}

			GetEventDispatcher().Dispatch(EventType::VendingOfferUpdated, &payload);
		}
	}
	shopFSM->DispatchEvent("Shop_Select");
	return true;
}