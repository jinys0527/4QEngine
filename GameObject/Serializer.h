#pragma once
#include <string>
#include <vector>
#include "json.hpp"
#include "RenderData.h"
#include "ResourceRefs.h"
#include "CameraSettings.h"
#include "AssetLoader.h"
#include "ResourceHandle.h"
#include "MeshComponent.h"
#include "FSMComponent.h"
#include "AnimationComponent.h"

//using namespace std;  <<- 이거쓰면 byte가 모호하다는 에러 발생 이유는 모름.;
using namespace MathUtils;


template<typename T>
struct Serializer {
	static void ToJson(nlohmann::json& j, const T& v);
	static void FromJson(const nlohmann::json& j, T& v);
};

template<>
struct Serializer<int> {
	static void ToJson(nlohmann::json& j, const int& v) { j = v; }
	static void FromJson(const nlohmann::json& j, int& v) { v = j.get<int>(); }

};
//float
template<>
struct Serializer<float> {
	static void ToJson(nlohmann::json& j, const float& v) { j = v; }
	static void FromJson(const nlohmann::json& j, float& v) { v = j.get<float>(); }
};

template<>
struct Serializer<bool> {
	static void ToJson(nlohmann::json& j, const bool& v) { j = v; }
	static void FromJson(const nlohmann::json& j, bool& v) { v = j.get<bool>(); }
};

template<>
struct Serializer<std::string> {
	static void ToJson(nlohmann::json& j, const std::string& v) { j = v; }
	static void FromJson(const nlohmann::json& j, std::string& v) { v = j.get<std::string>(); }
};

template<>
struct Serializer<XMFLOAT2> {
	static void ToJson(nlohmann::json& j, const XMFLOAT2& v) {
		j = { {"x", v.x}, {"y", v.y}};
	}

	static void FromJson(const nlohmann::json& j, XMFLOAT2& v) {
		v.x = j["x"];
		v.y = j["y"];
		
	}
};
//예시 인데 이러면 모든 구조체 마다 다 작성해야함.
//struct MyStruct { int a; float b; };
// MyStruct2 
//
//template<>
//struct Serializer<MyStruct> {
//	static void ToJson(nlohmann::json& j, const MyStruct& v) {
//		j = { {"a", v.a}, {"b", v.b} };
//	}
//	static void FromJson(const nlohmann::json& j, MyStruct& v) {
//		v.a = j.at("a").get<int>();
//		v.b = j.at("b").get<float>();
//	}
//};

//enum class MyEnum { A, B };
//
//template<>
//struct Serializer<MyEnum> {
//	static void ToJson(nlohmann::json& j, const MyEnum& v) { j = static_cast<int>(v); }
//	static void FromJson(const nlohmann::json& j, MyEnum& v) { v = static_cast<MyEnum>(j.get<int>()); }
//};

// Camera
template<>
struct Serializer<Viewport> {
	static void ToJson(nlohmann::json& j, const Viewport& v) {
		j["width"]  = v.Width;
		j["height"] = v.Height;
	}
	static void FromJson(const nlohmann::json& j, Viewport& v) {
		v.Width  = j.value("width",  0.0f);
		v.Height = j.value("height", 0.0f);
	}
};

template<>
struct Serializer<PerspectiveParams> {
	static void ToJson(nlohmann::json& j, const PerspectiveParams& v) {
		j["fov"] = v.Fov;
		j["aspect"] = v.Aspect;
	}
	static void FromJson(const nlohmann::json& j, PerspectiveParams& v) {
		v.Fov    = j.value("fov", XM_PIDIV4);
		v.Aspect = j.value("aspect", 1.0f);
	}
};

template<>
struct Serializer<OrthoParams> {
	static void ToJson(nlohmann::json& j, const OrthoParams& v) {
		j["width"]  = v.Width;
		j["height"] = v.Height;
	}
	static void FromJson(const nlohmann::json& j, OrthoParams& v) {
		v.Width  = j.value("width",  0.0f);
		v.Height = j.value("height", 0.0f);
	}
};

template<>
struct Serializer<OrthoOffCenterParams> {
	static void ToJson(nlohmann::json& j, const OrthoOffCenterParams& v) {
		j["left"]   = v.Left;
		j["right"]  = v.Right;
		j["bottom"] = v.Bottom;
		j["top"]	= v.Top;
	}
	static void FromJson(const nlohmann::json& j, OrthoOffCenterParams& v) {
		v.Left   = j.value("left", 0.0f);
		v.Right  = j.value("right", 0.0f);
		v.Bottom = j.value("bottom", 0.0f);
		v.Top	 = j.value("top", 0.0f);
	}
};

template<>
struct Serializer<ProjectionMode> {
	static void ToJson(nlohmann::json& j, const ProjectionMode& v) {
		j = static_cast<int>(v);
	}
	static void FromJson(const nlohmann::json& j, ProjectionMode& v) {
		v = static_cast<ProjectionMode>(j.get<int>());
	}
};

// AssetRef
template<> 
struct Serializer<AssetRef> {
	static void ToJson(nlohmann::json& j, const AssetRef& v) {
		j["assetPath"]  = v.assetPath;
		j["assetIndex"] = v.assetIndex;
	}
	static void FromJson(const nlohmann::json& j, AssetRef& v) {
		v.assetPath  = j.value("assetPath", std::string{});
		v.assetIndex = j.value("assetIndex", 0u);
	}
};

template<>
struct Serializer<std::vector<AssetRef>> {
	static void ToJson(nlohmann::json& j, const std::vector<AssetRef>& v) {
		j = nlohmann::json::array();
		for (const auto& item : v) {
			nlohmann::json entry;
			Serializer<AssetRef>::ToJson(entry, item);
			j.push_back(std::move(entry));
		}
	}

	static void FromJson(const nlohmann::json& j, std::vector<AssetRef>& v) {
		v.clear();
		if (!j.is_array()) {
			return;
		}
		v.reserve(j.size());
		for (const auto& entry : j) {
			AssetRef item{};
			Serializer<AssetRef>::FromJson(entry, item);
			v.push_back(std::move(item));
		}
	}
};

// Handle
template<>
struct Serializer<MeshHandle> {
	static void ToJson(nlohmann::json& j, const MeshHandle& v) {
		MeshRef ref{};
		if (auto* loader = AssetLoader::GetActive())
		{
			loader->GetMeshAssetReference(v, ref.assetPath, ref.assetIndex);
		}
		Serializer<MeshRef>::ToJson(j, ref);
	}

	static void FromJson(const nlohmann::json& j, MeshHandle& v) {
		MeshRef ref{};
		Serializer<MeshRef>::FromJson(j, ref);
		if (ref.assetPath.empty())
		{
			v = MeshHandle::Invalid();
			return;
		}

		if (auto* loader = AssetLoader::GetActive())
			v = loader->ResolveMesh(ref.assetPath, ref.assetIndex);
		else
			v = MeshHandle::Invalid();
	}
};

template<>
struct Serializer<MaterialHandle> {
	static void ToJson(nlohmann::json& j, const MaterialHandle& v) {
		MaterialRef ref{};
		if (auto* loader = AssetLoader::GetActive())
		{
			loader->GetMaterialAssetReference(v, ref.assetPath, ref.assetIndex);
		}
		Serializer<MaterialRef>::ToJson(j, ref);
	}

	static void FromJson(const nlohmann::json& j, MaterialHandle& v) {
		MaterialRef ref{};
		Serializer<MaterialRef>::FromJson(j, ref);
		if (ref.assetPath.empty())
		{
			v = MaterialHandle::Invalid();
			return;
		}

		if (auto* loader = AssetLoader::GetActive())
			v = loader->ResolveMaterial(ref.assetPath, ref.assetIndex);
		else
			v = MaterialHandle::Invalid();
	}
};

template<>
struct Serializer<TextureHandle> {
	static void ToJson(nlohmann::json& j, const TextureHandle& v) {
		TextureRef ref{};
		if (auto* loader = AssetLoader::GetActive())
		{
			loader->GetTextureAssetReference(v, ref.assetPath, ref.assetIndex);
		}
		Serializer<TextureRef>::ToJson(j, ref);
	}

	static void FromJson(const nlohmann::json& j, TextureHandle& v) {
		TextureRef ref{};
		Serializer<TextureRef>::FromJson(j, ref);
		if (ref.assetPath.empty())
		{
			v = TextureHandle::Invalid();
			return;
		}

		if (auto* loader = AssetLoader::GetActive())
			v = loader->ResolveTexture(ref.assetPath, ref.assetIndex);
		else
			v = TextureHandle::Invalid();
	}
};



template<>
struct Serializer<SkeletonHandle> {
	static void ToJson(nlohmann::json& j, const SkeletonHandle& v) {
		SkeletonRef ref{};
		if (auto* loader = AssetLoader::GetActive())
		{
			loader->GetSkeletonAssetReference(v, ref.assetPath, ref.assetIndex);
		}
		Serializer<SkeletonRef>::ToJson(j, ref);
	}

	static void FromJson(const nlohmann::json& j, SkeletonHandle& v) {
		SkeletonRef ref{};
		Serializer<SkeletonRef>::FromJson(j, ref);
		if (ref.assetPath.empty())
		{
			v = SkeletonHandle::Invalid();
			return;
		}

		if (auto* loader = AssetLoader::GetActive())
			v = loader->ResolveSkeleton(ref.assetPath, ref.assetIndex);
		else
			v = SkeletonHandle::Invalid();
	}
};

template<>
struct Serializer<AnimationHandle> {
	static void ToJson(nlohmann::json& j, const AnimationHandle& v) {
		AnimationRef ref{};
		if (auto* loader = AssetLoader::GetActive())
		{
			loader->GetAnimationAssetReference(v, ref.assetPath, ref.assetIndex);
		}
		Serializer<AnimationRef>::ToJson(j, ref);
	}

	static void FromJson(const nlohmann::json& j, AnimationHandle& v) {
		AnimationRef ref{};
		Serializer<AnimationRef>::FromJson(j, ref);
		if (ref.assetPath.empty())
		{
			v = AnimationHandle::Invalid();
			return;
		}

		if (auto* loader = AssetLoader::GetActive())
			v = loader->ResolveAnimation(ref.assetPath, ref.assetIndex);
		else
			v = AnimationHandle::Invalid();
	}
};

template<>
struct Serializer<ShaderAssetHandle> {
	static void ToJson(nlohmann::json& j, const ShaderAssetHandle& v) {
		ShaderAssetRef ref{};
		if (auto* loader = AssetLoader::GetActive())
		{
			loader->GetShaderAssetReference(v, ref.assetPath, ref.assetIndex);
		}
		Serializer<ShaderAssetRef>::ToJson(j, ref);
	}

	static void FromJson(const nlohmann::json& j, ShaderAssetHandle& v) {
		ShaderAssetRef ref{};
		Serializer<ShaderAssetRef>::FromJson(j, ref);
		if (ref.assetPath.empty())
		{
			v = ShaderAssetHandle::Invalid();
			return;
		}

		if (auto* loader = AssetLoader::GetActive())
			v = loader->ResolveShaderAsset(ref.assetPath, ref.assetIndex);
		else
			v = ShaderAssetHandle::Invalid();
	}
};

template<>
struct Serializer<VertexShaderHandle> {
	static void ToJson(nlohmann::json& j, const VertexShaderHandle& v) {
		VertexShaderRef ref{};
		if (auto* loader = AssetLoader::GetActive())
		{
			loader->GetVertexShaderAssetReference(v, ref.assetPath, ref.assetIndex);
		}
		Serializer<VertexShaderRef>::ToJson(j, ref);
	}

	static void FromJson(const nlohmann::json& j, VertexShaderHandle& v) {
		VertexShaderRef ref{};
		Serializer<VertexShaderRef>::FromJson(j, ref);
		if (ref.assetPath.empty())
		{
			v = VertexShaderHandle::Invalid();
			return;
		}

		if (auto* loader = AssetLoader::GetActive())
			v = loader->ResolveVertexShader(ref);
		else
			v = VertexShaderHandle::Invalid();
	}
};

template<>
struct Serializer<PixelShaderHandle> {
	static void ToJson(nlohmann::json& j, const PixelShaderHandle& v) {
		PixelShaderRef ref{};
		if (auto* loader = AssetLoader::GetActive())
		{
			loader->GetPixelShaderAssetReference(v, ref.assetPath, ref.assetIndex);
		}
		Serializer<PixelShaderRef>::ToJson(j, ref);
	}

	static void FromJson(const nlohmann::json& j, PixelShaderHandle& v) {
		PixelShaderRef ref{};
		Serializer<PixelShaderRef>::FromJson(j, ref);
		if (ref.assetPath.empty())
		{
			v = PixelShaderHandle::Invalid();
			return;
		}

		if (auto* loader = AssetLoader::GetActive())
			v = loader->ResolvePixelShader(ref);
		else
			v = PixelShaderHandle::Invalid();
	}
};


// MaterialData
template<>
struct Serializer<RenderData::MaterialData> {
	static void ToJson(nlohmann::json& j, const RenderData::MaterialData& v) {
		j["overrides"]["baseColor"]["r"] = v.baseColor.x;
		j["overrides"]["baseColor"]["g"] = v.baseColor.y;
		j["overrides"]["baseColor"]["b"] = v.baseColor.z;
		j["overrides"]["baseColor"]["a"] = v.baseColor.w;
		j["overrides"]["metallic"]       = v.metallic;
		j["overrides"]["roughness"]      = v.roughness;
		j["overrides"]["saturation"] = v.saturation;
		j["overrides"]["lightness"] = v.lightness;

		// textures: TextureHandle[] -> TextureRef[] 로 저장
		auto& outTex = j["overrides"]["textures"];
		outTex = nlohmann::json::array();

		if (auto* loader = AssetLoader::GetActive())
		{
			for (const auto& h : v.textures)
			{
				TextureRef ref{};
				if (h.IsValid())
				{
					loader->GetTextureAssetReference(h, ref.assetPath, ref.assetIndex);
				}
				nlohmann::json refJ;
				Serializer<TextureRef>::ToJson(refJ, ref);
				outTex.push_back(std::move(refJ));
			}
		}
		else
		{
			// 로더 없으면 빈 ref로 맞춰서 길이는 유지
			for (size_t i = 0; i < v.textures.size(); ++i)
			{
				nlohmann::json refJ;
				Serializer<TextureRef>::ToJson(refJ, TextureRef{});
				outTex.push_back(std::move(refJ));
			}
		}

		VertexShaderRef vertexRef{};
		PixelShaderRef pixelRef{};
		ShaderAssetRef shaderAssetRef{};
		if (auto* loader = AssetLoader::GetActive())
		{
			if (v.shaderAsset.IsValid())
				loader->GetShaderAssetReference(v.shaderAsset, shaderAssetRef.assetPath, shaderAssetRef.assetIndex);
			if (v.vertexShader.IsValid())
				loader->GetVertexShaderAssetReference(v.vertexShader, vertexRef.assetPath, vertexRef.assetIndex);
			if (v.pixelShader.IsValid())
				loader->GetPixelShaderAssetReference(v.pixelShader, pixelRef.assetPath, pixelRef.assetIndex);
		}
		Serializer<ShaderAssetRef>::ToJson(j["overrides"]["shaderAsset"], shaderAssetRef);
		Serializer<VertexShaderRef>::ToJson(j["overrides"]["vertexShader"], vertexRef);
		Serializer<PixelShaderRef>::ToJson(j["overrides"]["pixelShader"], pixelRef);
	}

	static void FromJson(const nlohmann::json& j, RenderData::MaterialData& v) {
		v.baseColor.x = j["overrides"]["baseColor"].value("r", 1.0f);
		v.baseColor.y = j["overrides"]["baseColor"].value("g", 1.0f);
		v.baseColor.z = j["overrides"]["baseColor"].value("b", 1.0f);
		v.baseColor.w = j["overrides"]["baseColor"].value("a", 1.0f);
		v.metallic    = j["overrides"].value("metallic", 0.0f);
		v.roughness   = j["overrides"].value("roughness", 1.0f);
		v.saturation = j["overrides"].value("saturation", 1.0f);
		v.saturation = j["overrides"].value("lightness", 1.0f);

		// textures: TextureRef[] -> TextureHandle[] 로 복원
		if (j["overrides"].contains("textures") && j["overrides"]["textures"].is_array())
		{
			const auto& arr = j["overrides"]["textures"];
			const size_t count = min(arr.size(), v.textures.size());

			for (size_t i = 0; i < count; ++i)
			{
				TextureRef ref{};
				Serializer<TextureRef>::FromJson(arr[i], ref);

				if (ref.assetPath.empty())
				{
					v.textures[i] = TextureHandle::Invalid();
					continue;
				}

				if (auto* loader = AssetLoader::GetActive())
					v.textures[i] = loader->ResolveTexture(ref.assetPath, ref.assetIndex);
				else
					v.textures[i] = TextureHandle::Invalid();
			}
		}

		if (j["overrides"].contains("vertexShader"))
		{
			VertexShaderRef ref{};
			Serializer<VertexShaderRef>::FromJson(j["overrides"]["vertexShader"], ref);
			if (auto* loader = AssetLoader::GetActive())
				v.vertexShader = loader->ResolveVertexShader(ref);
			else
				v.vertexShader = VertexShaderHandle::Invalid();
		}

		if (j["overrides"].contains("shaderAsset"))
		{
			ShaderAssetRef ref{};
			Serializer<ShaderAssetRef>::FromJson(j["overrides"]["shaderAsset"], ref);
			if (auto* loader = AssetLoader::GetActive())
				v.shaderAsset = loader->ResolveShaderAsset(ref.assetPath, ref.assetIndex);
			else
				v.shaderAsset = ShaderAssetHandle::Invalid();
		}

		if (j["overrides"].contains("pixelShader"))
		{
			PixelShaderRef ref{};
			Serializer<PixelShaderRef>::FromJson(j["overrides"]["pixelShader"], ref);
			if (auto* loader = AssetLoader::GetActive())
				v.pixelShader = loader->ResolvePixelShader(ref);
			else
				v.pixelShader = PixelShaderHandle::Invalid();
		}
	}
};

template<>
struct Serializer<MeshComponent::SubMeshMaterialOverride> {
	static void ToJson(nlohmann::json& j, const MeshComponent::SubMeshMaterialOverride& v) {
		Serializer<MaterialRef>::ToJson(j["material"], v.material);
		Serializer<ShaderAssetHandle>::ToJson(j["shaderAsset"], v.shaderAsset);
		Serializer<VertexShaderHandle>::ToJson(j["vertexShader"], v.vertexShader);
		Serializer<PixelShaderHandle>::ToJson(j["pixelShader"], v.pixelShader);
	}

	static void FromJson(const nlohmann::json& j, MeshComponent::SubMeshMaterialOverride& v) {
		if (j.contains("material")) {
			Serializer<MaterialRef>::FromJson(j["material"], v.material);
		}
		else {
			v.material = MaterialRef{};
		}

		if (j.contains("shaderAsset")) {
			Serializer<ShaderAssetHandle>::FromJson(j["shaderAsset"], v.shaderAsset);
		}
		else {
			v.shaderAsset = ShaderAssetHandle::Invalid();
		}

		if (j.contains("vertexShader")) {
			Serializer<VertexShaderHandle>::FromJson(j["vertexShader"], v.vertexShader);
		}
		else {
			v.vertexShader = VertexShaderHandle::Invalid();
		}

		if (j.contains("pixelShader")) {
			Serializer<PixelShaderHandle>::FromJson(j["pixelShader"], v.pixelShader);
		}
		else {
			v.pixelShader = PixelShaderHandle::Invalid();
		}
	}
};

template<>
struct Serializer<std::vector<MeshComponent::SubMeshMaterialOverride>> {
	static void ToJson(nlohmann::json& j, const std::vector<MeshComponent::SubMeshMaterialOverride>& v) {
		j = nlohmann::json::array();
		for (const auto& item : v) {
			nlohmann::json entry;
			Serializer<MeshComponent::SubMeshMaterialOverride>::ToJson(entry, item);
			j.push_back(std::move(entry));
		}
	}

	static void FromJson(const nlohmann::json& j, std::vector<MeshComponent::SubMeshMaterialOverride>& v) {
		v.clear();
		if (!j.is_array()) {
			return;
		}
		v.reserve(j.size());
		for (const auto& entry : j) {
			MeshComponent::SubMeshMaterialOverride item{};
			Serializer<MeshComponent::SubMeshMaterialOverride>::FromJson(entry, item);
			v.push_back(std::move(item));
		}
	}
};


// FSM
template<>
struct Serializer<FSMAction> {
	static void ToJson(nlohmann::json& j, const FSMAction& v) {
		j["id"]     = v.id;
		j["params"] = v.params;
	}

	static void FromJson(const nlohmann::json& j, FSMAction& v) {
		v.id     = j.value("id", std::string());
		v.params = j.value("params", nlohmann::json::object());
	}
};

template<>
struct Serializer<FSMTransition> {
	static void ToJson(nlohmann::json& j, const FSMTransition& v) {
		j["event"]    = v.eventName;
		j["target"]   = v.targetState;
		j["priority"] = v.priority;
	}

	static void FromJson(const nlohmann::json& j, FSMTransition& v) {
		v.eventName   = j.value("event", std::string());
		v.targetState = j.value("target", std::string());
		v.priority    = j.value("priority", 0);
	}
};

template<>
struct Serializer<std::vector<FSMAction>> {
	static void ToJson(nlohmann::json& j, const std::vector<FSMAction>& v) {
		j = nlohmann::json::array();
		for (const auto& item : v)
		{
			nlohmann::json entry;
			Serializer<FSMAction>::ToJson(entry, item);
			j.push_back(std::move(entry));
		}
	}

	static void FromJson(const nlohmann::json& j, std::vector<FSMAction>& v) {
		v.clear();
		if (!j.is_array())
			return;

		v.reserve(j.size());
		for (const auto& entry : j)
		{
			FSMAction item{};
			Serializer<FSMAction>::FromJson(entry, item);
			v.push_back(std::move(item));
		}
	}
};

template<>
struct Serializer<std::vector<FSMTransition>> {
	static void ToJson(nlohmann::json& j, const std::vector<FSMTransition>& v) {
		j = nlohmann::json::array();
		for (const auto& item : v)
		{
			nlohmann::json entry;
			Serializer<FSMTransition>::ToJson(entry, item);
			j.push_back(std::move(entry));
		}
	}

	static void FromJson(const nlohmann::json& j, std::vector<FSMTransition>& v) {
		v.clear();
		if (!j.is_array())
			return;

		v.reserve(j.size());
		for (const auto& entry : j)
		{
			FSMTransition item{};
			Serializer<FSMTransition>::FromJson(entry, item);
			v.push_back(std::move(item));
		}
	}
};

template<>
struct Serializer<FSMState> {
	static void ToJson(nlohmann::json& j, const FSMState& v) {
		j["name"] = v.name;
		Serializer<std::vector<FSMAction>>::ToJson(j["onEnter"], v.onEnter);
		Serializer<std::vector<FSMAction>>::ToJson(j["onExit"], v.onExit);
		Serializer<std::vector<FSMTransition>>::ToJson(j["transitions"], v.transitions);
	}

	static void FromJson(const nlohmann::json& j, FSMState& v) {
		v.name = j.value("name", std::string());
		Serializer<std::vector<FSMAction>>::FromJson(j.value("onEnter", nlohmann::json::array()), v.onEnter);
		Serializer<std::vector<FSMAction>>::FromJson(j.value("onExit", nlohmann::json::array()), v.onExit);
		Serializer<std::vector<FSMTransition>>::FromJson(j.value("transitions", nlohmann::json::array()), v.transitions);
	}
};



template<>
struct Serializer<std::vector<FSMState>> {
	static void ToJson(nlohmann::json& j, const std::vector<FSMState>& v) {
		j = nlohmann::json::array();
		for (const auto& item : v)
		{
			nlohmann::json entry;
			Serializer<FSMState>::ToJson(entry, item);
			j.push_back(std::move(entry));
		}
	}

	static void FromJson(const nlohmann::json& j, std::vector<FSMState>& v) {
		v.clear();
		if (!j.is_array())
			return;

		v.reserve(j.size());
		for (const auto& entry : j)
		{
			FSMState item{};
			Serializer<FSMState>::FromJson(entry, item);
			v.push_back(std::move(item));
		}
	}
};

template<>
struct Serializer<FSMGraph> {
	static void ToJson(nlohmann::json& j, const FSMGraph& v) {
		j["initialState"] = v.initialState;
		Serializer<std::vector<FSMState>>::ToJson(j["states"], v.states);
	}

	static void FromJson(const nlohmann::json& j, FSMGraph& v) {
		v.initialState = j.value("initialState", std::string());
		Serializer<std::vector<FSMState>>::FromJson(j.value("states", nlohmann::json::array()), v.states);;
	}
};

template<>
struct Serializer<AnimationComponent::BlendConfig> {
	static void ToJson(nlohmann::json& j, const AnimationComponent::BlendConfig& v) {
		Serializer<AnimationHandle>::ToJson(j["fromClip"], v.fromClip);
		Serializer<AnimationHandle>::ToJson(j["toClip"], v.toClip);
		j["blendTime"] = v.blendTime;
		j["blendType"] = static_cast<int>(v.blendType);
		j["curveName"] = v.curveName;
	}

	static void FromJson(const nlohmann::json& j, AnimationComponent::BlendConfig& v) {
		if (j.contains("fromClip")) {
			Serializer<AnimationHandle>::FromJson(j["fromClip"], v.fromClip);
		}
		if (j.contains("toClip")) {
			Serializer<AnimationHandle>::FromJson(j["toClip"], v.toClip);
		}
		v.blendTime = j.value("blendTime", 0.2f);
		v.blendType = static_cast<AnimationComponent::BlendType>(j.value("blendType", 0));
		v.curveName = j.value("curveName", std::string("Linear"));
	}
};

template<>
struct Serializer<UINT8> {
	static void ToJson(nlohmann::json& j, const UINT8& v) {
		j = static_cast<int>(v);
	}

	static void FromJson(const nlohmann::json& j, UINT8& v) {
		v = static_cast<UINT8>(j.get<int>());
	}
};


template<>
struct Serializer<XMFLOAT3> {
	static void ToJson(nlohmann::json& j, const XMFLOAT3& v) {
		j = { {"x", v.x}, {"y", v.y}, {"z", v.z} };
	}

	static void FromJson(const nlohmann::json& j, XMFLOAT3& v) {
		v.x = j["x"];
		v.y = j["y"];
		v.z = j["z"];
	}
};

template<>
struct Serializer<XMFLOAT4> {
	static void ToJson(nlohmann::json& j, const XMFLOAT4& v) {
		j = { {"x", v.x}, {"y", v.y}, {"z", v.z},{"w", v.w} };
	}

	static void FromJson(const nlohmann::json& j, XMFLOAT4& v) {
		v.x = j["x"];
		v.y = j["y"];
		v.z = j["z"];
		v.w = j["w"];
	}
};

template<>
struct Serializer<XMFLOAT4X4> {
	static void ToJson(nlohmann::json& j, const XMFLOAT4X4& v) {

	}

	static void FromJson(const nlohmann::json& j, XMFLOAT4X4& v) {

	}
};

template<>
struct Serializer<std::vector<XMFLOAT4X4>> {
	static void ToJson(nlohmann::json& j, const std::vector<XMFLOAT4X4>& v) {

	}

	static void FromJson(const nlohmann::json& j, std::vector<XMFLOAT4X4>& v) {

	}
};



// Draw ??