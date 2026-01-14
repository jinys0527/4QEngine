#pragma once
#include "Component.h"
#include "MathHelper.h"
#include "CameraSettings.h"

bool SceneHasObjectName(const Scene& scene, const std::string& name);
std::string MakeUniqueObjectName(const Scene& scene, const std::string& baseName);
void CopyStringToBuffer(const std::string& value, std::array<char, 256>& buffer);
bool DrawSubMeshOverridesEditor(MeshComponent& meshComponent, AssetLoader& assetLoader);
bool DrawComponentPropertyEditor(Component* component, const Property& property, AssetLoader& assetLoader);
