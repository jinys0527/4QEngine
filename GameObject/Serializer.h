#pragma once
#include <string>
#include "json.hpp"
#include "RenderData.h"
#include "ResourceRefs.h"
#include "CameraSettings.h"
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



// TextureHandle
template<>
struct Serializer<TextureHandle> {
	static void ToJson(nlohmann::json& j, const TextureHandle& v) {
		
	}

	static void FromJson(const nlohmann::json& j, TextureHandle& v) {

	}
};
// ShaderHandle

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
	}

	static void FromJson(const nlohmann::json& j, RenderData::MaterialData& v) {
		v.baseColor.x = j["overrides"]["baseColor"].value("r", 1.0f);
		v.baseColor.y = j["overrides"]["baseColor"].value("g", 1.0f);
		v.baseColor.z = j["overrides"]["baseColor"].value("b", 1.0f);
		v.baseColor.w = j["overrides"]["baseColor"].value("a", 1.0f);
		v.metallic    = j["overrides"].value("metallic", 0.0f);
		v.roughness   = j["overrides"].value("roughness", 1.0f);
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

// Draw ??