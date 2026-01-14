#pragma once
#include <string>
#include <vector>
#include "json.hpp"
#include "RenderData.h"
#include "ResourceRefs.h"
#include "CameraSettings.h"
#include "AssetLoader.h"
#include "ResourceHandle.h"

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

// 		// shader도 같은 방식 (ShaderHandle serializer가 없다면 ShaderRef로)
// 		ShaderRef sref{};
// 		if (auto* loader = AssetLoader::GetActive())
// 		{
// 			if (v.shader.IsValid())
// 				loader->GetShaderAssetReference(v.shader, sref.assetPath, sref.assetIndex); // 함수 필요
// 		}
// 		Serializer<ShaderRef>::ToJson(j["overrides"]["shader"], sref);
	}

	static void FromJson(const nlohmann::json& j, RenderData::MaterialData& v) {
		v.baseColor.x = j["overrides"]["baseColor"].value("r", 1.0f);
		v.baseColor.y = j["overrides"]["baseColor"].value("g", 1.0f);
		v.baseColor.z = j["overrides"]["baseColor"].value("b", 1.0f);
		v.baseColor.w = j["overrides"]["baseColor"].value("a", 1.0f);
		v.metallic    = j["overrides"].value("metallic", 0.0f);
		v.roughness   = j["overrides"].value("roughness", 1.0f);

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

		// shader도 동일
// 		if (ovr.contains("shader"))
// 		{
// 			ShaderRef sref{};
// 			Serializer<ShaderRef>::FromJson(ovr["shader"], sref);
// 
// 			if (auto* loader = AssetLoader::GetActive())
// 				v.shader = loader->ResolveShader(sref.assetPath, sref.assetIndex); // 함수 필요
// 			else
// 				v.shader = ShaderHandle::Invalid();
// 		}
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