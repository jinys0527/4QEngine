#pragma once
#include "json.hpp"
#include <filesystem>
#include <string>
#include <array>
#include "GameObject.h"
#include <memory>
#include "Scene.h"
#include <fstream>

namespace fs = std::filesystem;

struct ObjectSnapshot
{
	nlohmann::json data;
	bool isOpaque;
};

struct SceneStateSnapshot
{
	bool hasScene = false;
	nlohmann::json data;
	fs::path currentPath;
	fs::path selectedPath;
	std::string selectedObjectName;
	std::string lastSelectedObjectName;
	std::array<char, 256> objectNameBuffer;
	std::string lastSceneName;
	std::array<char, 256> sceneNameBuffer;
};

struct SceneFileSnapshot
{
	fs::path path;
	bool existed = false;
	std::string contents;
};

std::shared_ptr<GameObject> FindSceneObject(Scene* scene, const std::string& name, bool* isOpaque)
{
	if (!scene)
	{
		return nullptr;
	}

	const auto& opaqueObjects = scene->GetOpaqueObjects();
	if (auto it = opaqueObjects.find(name); it != opaqueObjects.end())
	{
		if (isOpaque)
		{
			*isOpaque = true;
		}
		return it->second;
	}

	const auto& transparentObjects = scene->GetTransparentObjects();
	if (auto it = transparentObjects.find(name); it != transparentObjects.end())
	{
		if (isOpaque)
		{
			*isOpaque = false;
		}
		return it->second;
	}

	return nullptr;
}

std::shared_ptr<GameObject> EnsureSceneObjects(Scene* scene, const ObjectSnapshot& snapshot)
{
	if (!scene)
		return nullptr;

	const std::string name = snapshot.data.value("name", "");
	if (name.empty())
	{
		return nullptr;
	}

	if (auto existing = FindSceneObject(scene, name, nullptr))
	{
		return existing;
	}

	auto created = std::make_shared<GameObject>(scene->GetEventDispatcher());
	created->Deserialize(snapshot.data);
	scene->AddGameObject(created, snapshot.isOpaque);
	return created;
}

void ApplySnapshot(Scene* scene, const ObjectSnapshot& snapshot)
{
	auto target = EnsureSceneObjects(scene, snapshot);
	if (!target)
		return;

	target->Deserialize(snapshot.data);
}

size_t MakePropertyKey(const Component* component, const std::string& propertyName)
{
	const size_t pointerHash = std::hash<const void*>{}(component);
	const size_t nameHash = std::hash<std::string>{}(propertyName);
	return pointerHash ^ (nameHash + 0x9e3779b97f4a7c15ULL + (pointerHash << 6) + (pointerHash >> 2));
}

SceneFileSnapshot CaptureFileSnapshot(const fs::path& path)
{
	SceneFileSnapshot snapshot;
	snapshot.path = path;
	if (path.empty())
		return snapshot;

	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		return snapshot;
	}

	snapshot.existed = true;
	snapshot.contents.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	return snapshot;
}

void RestoreFileSnapshot(const SceneFileSnapshot& snapshot)
{
	if(snapshot.path.empty())
	{
		return;
	}

	if (!snapshot.existed)
	{
		std::error_code error;
		fs::remove(snapshot.path, error);
		return;
	}

	std::ofstream ofs(snapshot.path);
	if (!ofs.is_open())
	{
		return;
	}
	ofs << snapshot.contents;
}