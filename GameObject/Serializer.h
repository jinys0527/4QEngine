#pragma once
#include <string>
#include "json.hpp"

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