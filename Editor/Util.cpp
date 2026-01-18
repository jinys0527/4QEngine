#include "pch.h"
#include "EditorApplication.h"
#include "CameraObject.h"
#include "CameraComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "MaterialComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMeshRenderer.h"
#include "AnimationComponent.h"
#include "TransformComponent.h"
#include "LightComponent.h"
#include "GameObject.h"
#include "Reflection.h"
#include "Renderer.h"
#include "Scene.h"
#include "DX11.h"
#include "Util.h"
#include "json.hpp"

#define DRAG_SPEED 0.01f
bool SceneHasObjectName(const Scene& scene, const std::string& name)
{
	const auto& opaqueObjects = scene.GetOpaqueObjects();
	const auto& transparentObjects = scene.GetTransparentObjects();
	return opaqueObjects.find(name) != opaqueObjects.end() || transparentObjects.find(name) != transparentObjects.end();
}

// 이름 변경용 helper 
void CopyStringToBuffer(const std::string& value, std::array<char, 256>& buffer)
{
	std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
}

XMFLOAT3 QuaternionToEulerRadians(const XMFLOAT4& quaternion)
{
	const XMVECTOR qNormalized = XMQuaternionNormalize(XMLoadFloat4(&quaternion));
	XMMATRIX rotationMatrix = XMMatrixRotationQuaternion(qNormalized);
	XMFLOAT4X4 m{};
	XMStoreFloat4x4(&m, rotationMatrix);

	const float sinPitch = -m._32;
	const float pitch = std::asin(std::clamp(sinPitch, -1.0f, 1.0f));
	const float cosPitch = std::cos(pitch);

	float roll = 0.0f;
	float yaw = 0.0f;

	if (std::abs(cosPitch) > 1e-4f)
	{
		roll = std::atan2(m._12, m._22);
		yaw = std::atan2(m._31, m._33);
	}
	else
	{
		roll = std::atan2(-m._21, m._11);
		yaw = 0.0f;
	}

	return { pitch, yaw, roll };
}

XMFLOAT4 EulerRadiansToQuaternion(const XMFLOAT3& eulerRadians)
{
	XMFLOAT4 result{};
	XMStoreFloat4(&result, XMQuaternionRotationRollPitchYaw(eulerRadians.x, eulerRadians.y, eulerRadians.z));
	return result;
}

// 이름 중복방지(넘버링)
std::string MakeUniqueObjectName(const Scene& scene, const std::string& baseName)
{
	if (!SceneHasObjectName(scene, baseName))
	{
		return baseName;
	}
	for (int index = 1; index < 10000; ++index)
	{
		std::string candidate = baseName + std::to_string(index);
		if (!SceneHasObjectName(scene, candidate))
		{
			return candidate;
		}
	}
	//return baseName + "_Overflow"; // 같은 이름이 10000개 넘을 리는 없을 것으로 생각. 일단 막아둠
}

float NormalizeDegrees(float degrees)
{
	float normalized = std::fmod(degrees, 360.0f);
	if (normalized > 180.0f)
	{
		normalized -= 360.0f;
	}
	else if (normalized < -180.0f)
	{
		normalized += 360.0f;
	}
	return normalized;
}

bool DrawSubMeshOverridesEditor(MeshComponent& meshComponent, AssetLoader& assetLoader)
{
	const MeshHandle meshHandle = meshComponent.GetMeshHandle();
	if (!meshHandle.IsValid())
	{
		ImGui::TextDisabled("SubMesh Overrides (mesh not set)");
		return false;
	}

	const auto* meshData = assetLoader.GetMeshes().Get(meshHandle);
	if (!meshData)
	{
		ImGui::TextDisabled("SubMesh Overrides (mesh not loaded)");
		return false;
	}

	const size_t subMeshCount = meshData->subMeshes.empty() ? 1 : meshData->subMeshes.size();
	const auto& overrides = meshComponent.GetSubMeshMaterialOverrides();
	bool changed = false;

	// Clear UI 상태(직전 Clear 누른 슬롯만 <None>으로 보이게)
	static MeshComponent* s_lastClearedComp = nullptr;
	static int s_lastClearedIndex = -1;

	auto resolveMaterialDisplay = [&assetLoader](const MaterialHandle& handle) -> std::string
		{
			if (!handle.IsValid())
				return {};

			// displayName 우선
			if (const std::string* displayName = assetLoader.GetMaterials().GetDisplayName(handle))
			{
				if (!displayName->empty())
					return *displayName;
			}

			if (const std::string* key = assetLoader.GetMaterials().GetKey(handle))
			{
				if (!key->empty())
					return *key;
			}

			return {};
		};

	// ★ 오브젝트(MaterialComponent) 쪽 "현재 베이스 머티리얼"을 먼저 잡아둠
	MaterialHandle ownerBaseMaterial = MaterialHandle::Invalid();
	if (const auto* owner = meshComponent.GetOwner())
	{
		if (const auto* materialComponent = owner->GetComponent<MaterialComponent>())
			ownerBaseMaterial = materialComponent->GetMaterialHandle();
	}

	if (ImGui::TreeNode("SubMesh Overrides"))
	{
		for (size_t i = 0; i < subMeshCount; ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			ImGui::AlignTextToFramePadding();

			std::string subMeshName;

			// ★ meshbin에 박힌 서브메시 기본 머티리얼 (fallback용)
			MaterialHandle subMeshDefaultMaterial = MaterialHandle::Invalid();
			if (!meshData->subMeshes.empty() && i < meshData->subMeshes.size())
			{
				subMeshName = meshData->subMeshes[i].name;
				subMeshDefaultMaterial = meshData->subMeshes[i].material;
			}

			// ★ UI에서 보여줄 baseMaterial 우선순위:
			// 1) 오브젝트(MaterialComponent) 현재 값
			// 2) meshbin의 submesh 기본값(임포트 당시)
			MaterialHandle baseMaterial = ownerBaseMaterial.IsValid() ? ownerBaseMaterial : subMeshDefaultMaterial;

			std::string baseDisplay = resolveMaterialDisplay(baseMaterial);

			if (!subMeshName.empty())
				ImGui::Text("%s", subMeshName.c_str());
			else
				ImGui::Text("SubMesh %zu", i);
			ImGui::SameLine();

			// override display 결정
			std::string overrideDisplay; // 비어있으면 override 없음
			if (i < overrides.size())
			{
				const auto& overrideRef = overrides[i].material;
				if (!overrideRef.assetPath.empty())
				{
					const MaterialHandle handle = assetLoader.ResolveMaterial(overrideRef.assetPath, overrideRef.assetIndex);
					if (handle.IsValid())
					{
						// displayName 우선
						if (const std::string* displayName = assetLoader.GetMaterials().GetDisplayName(handle))
						{
							if (!displayName->empty())
								overrideDisplay = *displayName;
						}
						if (overrideDisplay.empty())
						{
							if (const std::string* key = assetLoader.GetMaterials().GetKey(handle))
							{
								if (!key->empty())
									overrideDisplay = *key;
							}
						}
					}

					// resolve 실패/표시이름 없음이면 path라도 보여줌
					if (overrideDisplay.empty())
						overrideDisplay = overrideRef.assetPath;
				}
			}

			// Clear 상태 체크
			bool isClear = (s_lastClearedComp == &meshComponent && s_lastClearedIndex == (int)i);

			// shown 우선순위: overrideDisplay > baseDisplay > <None>
			const char* name =
				(!overrideDisplay.empty()) ? overrideDisplay.c_str() :
				isClear ? "<None>" :
				(!baseDisplay.empty()) ? baseDisplay.c_str() :
				"<None>";

			std::string buttonLabel = std::string(name) + "##SubMeshOverride";
			ImGui::Button(buttonLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_MATERIAL"))
				{
					const MaterialHandle dropped = *static_cast<const MaterialHandle*>(payload->Data);
					std::string assetPath;
					UINT32 assetIndex = 0;
					if (assetLoader.GetMaterialAssetReference(dropped, assetPath, assetIndex))
					{
						meshComponent.SetSubMeshMaterialOverride(i, MaterialRef{ assetPath, assetIndex });
						changed = true;

						// 드롭으로 override가 들어오면 Clear 상태 해제
						if (s_lastClearedComp == &meshComponent && s_lastClearedIndex == (int)i)
						{
							s_lastClearedComp = nullptr;
							s_lastClearedIndex = -1;
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				meshComponent.ClearSubMeshMaterialOverride(i);
				changed = true;
					
				s_lastClearedIndex = static_cast<int>(i);
				s_lastClearedComp = &meshComponent;
			}

			const auto* overrideData = (i < overrides.size()) ? &overrides[i] : nullptr;
			ShaderAssetHandle shaderAssetHandle = overrideData ? overrideData->shaderAsset : ShaderAssetHandle::Invalid();
			VertexShaderHandle vsHandle = overrideData ? overrideData->vertexShader : VertexShaderHandle::Invalid();
			PixelShaderHandle psHandle = overrideData ? overrideData->pixelShader : PixelShaderHandle::Invalid();

			auto resolveShaderAssetDisplay = [&assetLoader](const ShaderAssetHandle& handle) -> std::string
				{
					if (!handle.IsValid())
						return {};

					if (const std::string* displayName = assetLoader.GetShaderAssets().GetDisplayName(handle))
					{
						if (!displayName->empty())
							return *displayName;
					}

					if (const std::string* key = assetLoader.GetShaderAssets().GetKey(handle))
					{
						if (!key->empty())
							return *key;
					}

					return {};
				};


			auto resolveVertexShaderDisplay = [&assetLoader](const VertexShaderHandle& handle) -> std::string
				{
					if (!handle.IsValid())
						return {};

					if (const std::string* displayName = assetLoader.GetVertexShaders().GetDisplayName(handle))
					{
						if (!displayName->empty())
							return *displayName;
					}

					if (const std::string* key = assetLoader.GetVertexShaders().GetKey(handle))
					{
						if (!key->empty())
							return *key;
					}

					return {};
				};

			auto resolvePixelShaderDisplay = [&assetLoader](const PixelShaderHandle& handle) -> std::string
				{
					if (!handle.IsValid())
						return {};

					if (const std::string* displayName = assetLoader.GetPixelShaders().GetDisplayName(handle))
					{
						if (!displayName->empty())
							return *displayName;
					}

					if (const std::string* key = assetLoader.GetPixelShaders().GetKey(handle))
					{
						if (!key->empty())
							return *key;
					}

					return {};
				};

			std::string vsDisplay = resolveVertexShaderDisplay(vsHandle);
			std::string psDisplay = resolvePixelShaderDisplay(psHandle);
			std::string shaderAssetDisplay = resolveShaderAssetDisplay(shaderAssetHandle);

			const char* vsName = vsDisplay.empty() ? "<None>" : vsDisplay.c_str();
			const char* psName = psDisplay.empty() ? "<None>" : psDisplay.c_str();
			const char* shaderAssetName = shaderAssetDisplay.empty() ? "<None>" : shaderAssetDisplay.c_str();

			ImGui::Indent();
			{
				std::string vsButtonLabel = std::string(vsName) + "##SubMeshVSOverride";
				ImGui::TextUnformatted("Vertex Shader");
				ImGui::SameLine();
				ImGui::Button(vsButtonLabel.c_str());

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_VERTEX_SHADER"))
					{
						const VertexShaderHandle dropped = *static_cast<const VertexShaderHandle*>(payload->Data);
						meshComponent.SetSubMeshVertexShaderOverride(i, dropped);
						changed = true;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear##SubMeshVS"))
				{
					meshComponent.SetSubMeshVertexShaderOverride(i, VertexShaderHandle::Invalid());
					changed = true;
				}

				std::string psButtonLabel = std::string(psName) + "##SubMeshPSOverride";
				ImGui::TextUnformatted("Pixel Shader");
				ImGui::SameLine();
				ImGui::Button(psButtonLabel.c_str());

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_PIXEL_SHADER"))
					{
						const PixelShaderHandle dropped = *static_cast<const PixelShaderHandle*>(payload->Data);
						meshComponent.SetSubMeshPixelShaderOverride(i, dropped);
						changed = true;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear##SubMeshPS"))
				{
					meshComponent.SetSubMeshPixelShaderOverride(i, PixelShaderHandle::Invalid());
					changed = true;
				}

				std::string shaderAssetLabel = std::string(shaderAssetName) + "##SubMeshShaderAssetOverride";
				ImGui::TextUnformatted("Shader Asset");
				ImGui::SameLine();
				ImGui::Button(shaderAssetLabel.c_str());

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_SHADER_ASSET"))
					{
						const ShaderAssetHandle dropped = *static_cast<const ShaderAssetHandle*>(payload->Data);
						meshComponent.SetSubMeshShaderAssetOverride(i, dropped);
						changed = true;
					}
					ImGui::EndDragDropTarget();
				}

				ImGui::SameLine();
				if (ImGui::Button("Clear##SubMeshShaderAsset"))
				{
					meshComponent.SetSubMeshShaderAssetOverride(i, ShaderAssetHandle::Invalid());
					changed = true;
				}
			}
			ImGui::Unindent();


			ImGui::PopID();
		}
		ImGui::TreePop();
	}

	return changed;
}

bool DrawComponentPropertyEditor(Component* component, const Property& property, AssetLoader& assetLoader)
{	// 각 Property별 배치 Layout은 정해줘야 함
	using PlaybackStateType = std::decay_t<decltype(std::declval<AnimationComponent>().GetPlayback())>;
	using BoneMaskSourceType = std::decay_t<decltype(std::declval<AnimationComponent>().GetBoneMaskSource())>;
	using RetargetOffsetsType = std::decay_t<decltype(std::declval<AnimationComponent>().GetRetargetOffsets())>;
	const std::type_info& typeInfo = property.GetTypeInfo();

	struct RotationUIState
	{
		XMFLOAT4 lastQuaternion{ 0.0f, 0.0f, 0.0f, 1.0f };
		XMFLOAT3 eulerDegrees{ 0.0f, 0.0f, 0.0f };
		bool initialized = false;
	};

	static std::unordered_map<const void*, RotationUIState> rotationUiState;

	if (typeInfo == typeid(int))
	{
		int value = 0;
		property.GetValue(component, &value);
		if (ImGui::DragInt(property.GetName().c_str(), &value))
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(float))
	{
		float value = 0.0f;
		property.GetValue(component, &value);
		if (ImGui::DragFloat(property.GetName().c_str(), &value, DRAG_SPEED))
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(bool))
	{
		bool value = false;
		property.GetValue(component, &value);
		if (ImGui::Checkbox(property.GetName().c_str(), &value))
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(std::string))
	{
		std::string value;
		property.GetValue(component, &value);
		std::array<char, 256> buffer{};
		CopyStringToBuffer(value, buffer);
		if (ImGui::InputText(property.GetName().c_str(), buffer.data(), buffer.size()))
		{
			std::string updatedValue(buffer.data());
			property.SetValue(component, &updatedValue);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(XMFLOAT2))
	{
		XMFLOAT2 value{};
		property.GetValue(component, &value);
		float data[2] = { value.x, value.y };
		if (ImGui::DragFloat2(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.x = data[0];
			value.y = data[1];
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(XMFLOAT3))
	{ 
		if (auto* transform = dynamic_cast<LightComponent*>(component)) {
			if (property.GetName() == "Color") {
				XMFLOAT3 value{};
				property.GetValue(component, &value);
				float data[3] = { value.x, value.y, value.z };
				if (ImGui::ColorEdit3(property.GetName().c_str(), data, DRAG_SPEED))
				{
					value.x = data[0];
					value.y = data[1];
					value.z = data[2];
					property.SetValue(component, &value);
					return true;
				}
				return false;
			}

		}

		XMFLOAT3 value{};
		property.GetValue(component, &value);
		float data[3] = { value.x, value.y, value.z };
		if (ImGui::DragFloat3(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.x = data[0];
			value.y = data[1];
			value.z = data[2];
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(XMFLOAT4))
	{
		if (auto* transform = dynamic_cast<TransformComponent*>(component))
		{

			// Rotation에 대한 GUI만 오일러로 변환
			if (property.GetName() == "Rotation")
			{
				XMFLOAT4 value{};
				property.GetValue(component, &value);
				auto& state = rotationUiState[component];
				const float diff = std::abs(state.lastQuaternion.x - value.x)
					+ std::abs(state.lastQuaternion.y - value.y)
					+ std::abs(state.lastQuaternion.z - value.z)
					+ std::abs(state.lastQuaternion.w - value.w);

				if (!state.initialized || diff > 1e-4f)
				{
					const XMFLOAT3 eulerRadians = QuaternionToEulerRadians(value);
					const XMFLOAT3 eulerDegrees = {
						XMConvertToDegrees(eulerRadians.x),
						XMConvertToDegrees(eulerRadians.y),
						XMConvertToDegrees(eulerRadians.z)
					};
					state.eulerDegrees = {
						NormalizeDegrees(eulerDegrees.x),
						NormalizeDegrees(eulerDegrees.y),
						NormalizeDegrees(eulerDegrees.z)
					};
					state.lastQuaternion = value;
					state.initialized = true;
				}

				float data[3] = {
					state.eulerDegrees.x,
					state.eulerDegrees.y,
					state.eulerDegrees.z
				};

				if (ImGui::DragFloat3(property.GetName().c_str(), data, 0.1f))
				{
					state.eulerDegrees = {
						NormalizeDegrees(data[0]),
						NormalizeDegrees(data[1]),
						NormalizeDegrees(data[2])
					};
					const XMFLOAT3 updatedRadians = {
						XMConvertToRadians(state.eulerDegrees.x),
						XMConvertToRadians(state.eulerDegrees.y),
						XMConvertToRadians(state.eulerDegrees.z)
					};
					const XMFLOAT4 updatedRotation = EulerRadiansToQuaternion(updatedRadians);
					property.SetValue(component, &updatedRotation);
					state.lastQuaternion = updatedRotation;
					return true;
				}

				return false;
			}
		}

		XMFLOAT4 value{};
		property.GetValue(component, &value);
		float data[4] = { value.x, value.y, value.z, value.w };
		if (ImGui::DragFloat4(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.x = data[0];
			value.y = data[1];
			value.z = data[2];
			value.w = data[3];
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	// 카메라
	if (typeInfo == typeid(Viewport))
	{
		Viewport value{};
		property.GetValue(component, &value);
		float data[2] = { value.Width, value.Height };
		if (ImGui::DragFloat2(property.GetName().c_str(), data, DRAG_SPEED))
		{
			value.Width = data[0];
			value.Height = data[1];
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(PerspectiveParams))
	{
		PerspectiveParams value{};
		property.GetValue(component, &value);
		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());
		updated |= ImGui::InputFloat("Fov", &value.Fov);
		updated |= ImGui::InputFloat("Aspect", &value.Aspect);
		ImGui::PopID();
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(OrthoParams))
	{
		OrthoParams value{};
		property.GetValue(component, &value);
		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());
		updated |= ImGui::InputFloat("Width", &value.Width);
		updated |= ImGui::InputFloat("Height", &value.Height);
		ImGui::PopID();
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(OrthoOffCenterParams))
	{
		OrthoOffCenterParams value{};
		property.GetValue(component, &value);
		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());
		updated |= ImGui::InputFloat("Left", &value.Left);
		updated |= ImGui::InputFloat("Right", &value.Right);
		updated |= ImGui::InputFloat("Bottom", &value.Bottom);
		updated |= ImGui::InputFloat("Top", &value.Top);
		ImGui::PopID();
		ImGui::Unindent();
		if (updated)
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}


	// Render Layer
	if (typeInfo == typeid(UINT8))
	{
		UINT8 value = 0;
		property.GetValue(component, &value);
		if (property.GetName() == "RenderLayer")
		{
			static constexpr const char* kLayers[] = { "Opaque", "Transparent", "UI" };
			int current = static_cast<int>(value);
			if (ImGui::Combo(property.GetName().c_str(), &current, kLayers, IM_ARRAYSIZE(kLayers)))
			{
				const UINT8 updated = static_cast<UINT8>(std::clamp(current, 0, 2));
				property.SetValue(component, &updated);
				return true;
			}
			return false;
		}

		if (ImGui::DragScalar(property.GetName().c_str(), ImGuiDataType_U8, &value))
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}


	// AssetPath + AssetIndex
	if (typeInfo == typeid(AssetRef))
	{
		AssetRef value{};
		property.GetValue(component, &value);

		std::string label = property.GetName();
		label += ": ";
		if (!value.assetPath.empty())
		{
			label += value.assetPath + " [" + std::to_string(value.assetIndex) + "]";
		}
		else
		{
			label += "<None>";
		}
		ImGui::TextUnformatted(label.c_str());
		return false;
	}

	if (typeInfo == typeid(XMFLOAT4X4))
	{
		XMFLOAT4X4 value{};
		property.GetValue(component, &value);
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Text("  [%.3f %.3f %.3f %.3f]", value._11, value._12, value._13, value._14);
		ImGui::Text("  [%.3f %.3f %.3f %.3f]", value._21, value._22, value._23, value._24);
		ImGui::Text("  [%.3f %.3f %.3f %.3f]", value._31, value._32, value._33, value._34);
		ImGui::Text("  [%.3f %.3f %.3f %.3f]", value._41, value._42, value._43, value._44);
		return false;
	}


	if (typeInfo == typeid(MeshHandle))
	{
		MeshHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetMeshes().GetKey(value);
		const std::string* displayName = assetLoader.GetMeshes().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_MESH"))
			{
				const MeshHandle dropped = *static_cast<const MeshHandle*>(payload->Data);
				property.SetValue(component, &dropped);

				if (auto* meshComponent = dynamic_cast<MeshComponent*>(component))
				{
					const std::string* droppedKey = assetLoader.GetMeshes().GetKey(dropped);
				}

				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				return true;
			}
		}

		return false;
	}

	if (typeInfo == typeid(MaterialHandle))
	{
		MaterialHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetMaterials().GetKey(value);
		const std::string* displayName = assetLoader.GetMaterials().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_MATERIAL"))
			{
				const MaterialHandle dropped = *static_cast<const MaterialHandle*>(payload->Data);
				property.SetValue(component, &dropped);

				if (auto* materialComponent = dynamic_cast<MaterialComponent*>(component))
				{
					const std::string* droppedKey = assetLoader.GetMaterials().GetKey(dropped);
				}

				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				return true;
			}
		}

		return false;
	}

	if (typeInfo == typeid(TextureHandle))
	{
		TextureHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetTextures().GetKey(value);
		const std::string* displayName = assetLoader.GetTextures().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_TEXTURE"))
			{
				const TextureHandle dropped = *static_cast<const TextureHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				return true;
			}
		}

		return false;
	}

	if (typeInfo == typeid(AnimationHandle))
	{
		AnimationHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetAnimations().GetKey(value);
		const std::string* displayName = assetLoader.GetAnimations().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ANIMATION"))
			{
				const AnimationHandle dropped = *static_cast<const AnimationHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				return true;
			}
		}

		return false;
	}


	if (typeInfo == typeid(SkeletonHandle))
	{
		SkeletonHandle value{};
		property.GetValue(component, &value);

		const std::string* key = assetLoader.GetSkeletons().GetKey(value);
		const std::string* displayName = assetLoader.GetSkeletons().GetDisplayName(value);

		const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
		const std::string buttonLabel = std::string(name) + "##" + property.GetName();

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::SameLine();
		ImGui::Button(buttonLabel.c_str());

		if (ImGui::BeginDragDropTarget())
		{
			bool updated = false;
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_SKELETON"))
			{
				const SkeletonHandle dropped = *static_cast<const SkeletonHandle*>(payload->Data);
				property.SetValue(component, &dropped);
				updated = true;
			}
			ImGui::EndDragDropTarget();
			if (updated)
			{
				return true;
			}
		}

		return false;
	}



	if (typeInfo == typeid(RenderData::MaterialData))
	{
		RenderData::MaterialData value{};
		property.GetValue(component, &value);

		bool updated = false;
		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());

		updated |= ImGui::ColorEdit4("Base Color", &value.baseColor.x);
		updated |= ImGui::DragFloat("Metallic", &value.metallic, 0.01f, 0.0f, 1.0f);
		updated |= ImGui::DragFloat("Roughness", &value.roughness, 0.01f, 0.0f, 1.0f);

		// Texture slots
		static constexpr const char* kTextureLabels[] = { "Albedo", "Normal", "Metallic", "Roughness", "AO", "Env" };
		static_assert(std::size(kTextureLabels) == static_cast<size_t>(RenderData::MaterialTextureSlot::TEX_MAX));

		for (size_t i = 0; i < value.textures.size(); ++i)
		{
			ImGui::PushID(static_cast<int>(i));
			TextureHandle handle = value.textures[i];
			const std::string* key = assetLoader.GetTextures().GetKey(handle);
			const std::string* displayName = assetLoader.GetTextures().GetDisplayName(handle);

			const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
			const std::string buttonLabel = std::string(name) + "##MaterialTexture";

			ImGui::TextUnformatted(kTextureLabels[i]);
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_TEXTURE"))
				{
					const TextureHandle dropped = *static_cast<const TextureHandle*>(payload->Data);
					value.textures[i] = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##Texture"))
			{
				value.textures[i] = TextureHandle::Invalid();
				updated = true;
			}


			ImGui::PopID();
		}

		bool stageShaderChanged = false;

		// Vertex shader handle
		{
			VertexShaderHandle shader = value.vertexShader;
			const std::string* key = assetLoader.GetVertexShaders().GetKey(shader);
			const std::string* displayName = assetLoader.GetVertexShaders().GetDisplayName(shader);
			const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
			const std::string buttonLabel = std::string(name) + "##MaterialVertexShader";

			ImGui::TextUnformatted("Vertex Shader");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_VERTEX_SHADER"))
				{
					const VertexShaderHandle dropped = *static_cast<const VertexShaderHandle*>(payload->Data);
					value.vertexShader = dropped;
					stageShaderChanged = true;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##VertexShader"))
			{
				value.vertexShader = VertexShaderHandle::Invalid();
				stageShaderChanged = true;
				updated = true;
			}
		}

		// Pixel shader handle
		{
			PixelShaderHandle shader = value.pixelShader;
			const std::string* key = assetLoader.GetPixelShaders().GetKey(shader);
			const std::string* displayName = assetLoader.GetPixelShaders().GetDisplayName(shader);
			const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
			const std::string buttonLabel = std::string(name) + "##MaterialPixelShader";

			ImGui::TextUnformatted("Pixel Shader");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_PIXEL_SHADER"))
				{
					const PixelShaderHandle dropped = *static_cast<const PixelShaderHandle*>(payload->Data);
					value.pixelShader = dropped;
					stageShaderChanged = true;
					updated = true;
			
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##PixelShader"))
			{
				value.pixelShader = PixelShaderHandle::Invalid();
				stageShaderChanged = true;
				updated = true;
			}
		}

		// Shader asset handle
		{
			ShaderAssetHandle shader = value.shaderAsset;
			const std::string* key = assetLoader.GetShaderAssets().GetKey(shader);
			const std::string* displayName = assetLoader.GetShaderAssets().GetDisplayName(shader);
			const char* name = (displayName && !displayName->empty()) ? displayName->c_str() : (key && !key->empty()) ? key->c_str() : "<None>";
			const std::string buttonLabel = std::string(name) + "##MaterialShaderAsset";

			ImGui::TextUnformatted("Shader Asset");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_SHADER_ASSET"))
				{
					const ShaderAssetHandle dropped = *static_cast<const ShaderAssetHandle*>(payload->Data);
					value.shaderAsset = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##ShaderAsset"))
			{
				value.shaderAsset = ShaderAssetHandle::Invalid();
				updated = true;
			}
		}

		ImGui::PopID();
		ImGui::Unindent();

		if (updated && !value.shaderAsset.IsValid() && value.vertexShader.IsValid() && value.pixelShader.IsValid())
		{
			value.shaderAsset = ShaderAssetHandle::Invalid();
		}
		if (updated)
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}


	if (typeInfo == typeid(std::vector<float>))
	{
		std::vector<float> value;
		property.GetValue(component, &value);
		ImGui::Text("%s: %zu entries", property.GetName().c_str(), value.size());
		return false;
	}

	if (typeInfo == typeid(std::vector<DirectX::XMFLOAT4X4>))
	{
		std::vector<DirectX::XMFLOAT4X4> value;
		property.GetValue(component, &value);
		ImGui::Text("%s: %zu matrices", property.GetName().c_str(), value.size());
		return false;
	}


	if (typeInfo == typeid(PlaybackStateType))
	{
		PlaybackStateType value{};
		property.GetValue(component, &value);
		ImGui::Text("%s: time %.3f, speed %.2f, looping %s, playing %s",
			property.GetName().c_str(),
			value.time,
			value.speed,
			value.looping ? "true" : "false",
			value.playing ? "true" : "false");
		return false;
	}

	if (typeInfo == typeid(AnimationComponent::BlendState))
	{
		AnimationComponent::BlendState value{};
		property.GetValue(component, &value);

		bool updated = false;

		ImGui::TextUnformatted(property.GetName().c_str());
		ImGui::Indent();
		ImGui::PushID(property.GetName().c_str());

		// active
		updated |= ImGui::Checkbox("Active", &value.active);

		// fromClip
		{
			const std::string* key = assetLoader.GetAnimations().GetKey(value.fromClip);
			const std::string display = key ? *key : std::string("<None>");
			const std::string buttonLabel = display + "##BlendFromClip";

			ImGui::TextUnformatted("From Clip");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ANIMATION"))
				{
					const AnimationHandle dropped = *static_cast<const AnimationHandle*>(payload->Data);
					value.fromClip = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##BlendFromClip"))
			{
				value.fromClip = AnimationHandle::Invalid();
				updated = true;
			}
		}

		// toClip
		{
			const std::string* key = assetLoader.GetAnimations().GetKey(value.toClip);
			const std::string display = key ? *key : std::string("<None>");
			const std::string buttonLabel = display + "##BlendToClip";

			ImGui::TextUnformatted("To Clip");
			ImGui::SameLine();
			ImGui::Button(buttonLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE_ANIMATION"))
				{
					const AnimationHandle dropped = *static_cast<const AnimationHandle*>(payload->Data);
					value.toClip = dropped;
					updated = true;
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear##BlendToClip"))
			{
				value.toClip = AnimationHandle::Invalid();
				updated = true;
			}
		}

		// duration / elapsed / fromTime / toTime
		updated |= ImGui::InputFloat("Duration", &value.duration);
		updated |= ImGui::InputFloat("Elapsed", &value.elapsed);
		updated |= ImGui::InputFloat("From Time", &value.fromTime);
		updated |= ImGui::InputFloat("To Time", &value.toTime);

		// blendType (enum)
		{
			// enum -> int combo
			// BlendType 정의에 맞게 라벨/개수 수정 필요
			// 여기선 Linear, EaseIn, EaseOut, EaseInOut, Curve 가정
			static constexpr const char* kBlendTypeLabels[] =
			{
				"Linear",
				"EaseIn",
				"EaseOut",
				"EaseInOut",
				"Curve"
			};

			int current = static_cast<int>(value.blendType);
			if (ImGui::Combo("Blend Type", &current, kBlendTypeLabels, IM_ARRAYSIZE(kBlendTypeLabels)))
			{
				value.blendType = static_cast<AnimationComponent::BlendType>(current);
				updated = true;
			}
		}

		// curveFn: function pointer -> edit 불가 (표시만)
		{
			const bool hasCurve = (value.curveFn != nullptr);
			ImGui::Text("Curve Fn: %s", hasCurve ? "Set" : "None");
			// 필요하면 주소 표시 (디버그용)
			// ImGui::Text("Curve Fn Ptr: 0x%p", reinterpret_cast<void*>(value.curveFn));
		}

		ImGui::PopID();
		ImGui::Unindent();

		if (updated)
		{
			property.SetValue(component, &value);
			return true;
		}
		return false;
	}

	if (typeInfo == typeid(BoneMaskSourceType))
	{
		BoneMaskSourceType value{};
		property.GetValue(component, &value);
		const char* label = "None";
		switch (value)
		{
		case AnimationComponent::BoneMaskSource::UpperBody:
			label = "UpperBody";
			break;
		case AnimationComponent::BoneMaskSource::LowerBody:
			label = "LowerBody";
			break;
		default:
			break;
		}
		ImGui::Text("%s: %s", property.GetName().c_str(), label);
		return false;
	}

	if (typeInfo == typeid(RetargetOffsetsType))
	{
		RetargetOffsetsType value;
		property.GetValue(component, &value);
		ImGui::Text("%s: %zu offsets", property.GetName().c_str(), value.size());
		return false;
	}

	ImGui::TextDisabled("%s (Unsupported)", property.GetName().c_str());
	return false;
}

bool ProjectToViewport(
	const XMFLOAT3& world,
	const XMMATRIX& viewProj,
	const ImVec2& rectMin,
	const ImVec2& rectMax,
	ImVec2& out)
{
	const XMVECTOR projected = XMVector3TransformCoord(XMLoadFloat3(&world), viewProj);
	const float x = XMVectorGetX(projected);
	const float y = XMVectorGetY(projected);
	const float z = XMVectorGetZ(projected);

	if (z < 0.0f || z > 1.0f)
	{
		return false;
	}

	const ImVec2 rectSize = ImVec2(rectMax.x - rectMin.x, rectMax.y - rectMin.y);
	const float screenX = rectMin.x + (x * 0.5f + 0.5f) * rectSize.x;
	const float screenY = rectMin.y + (1.0f - (y * 0.5f + 0.5f)) * rectSize.y;
	out = ImVec2(screenX, screenY);
	return true;
}