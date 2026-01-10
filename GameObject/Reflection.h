#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include "MathHelper.h"
#include "Serializer.h"
#include "Component.h"
#include "json.hpp"

using namespace std;
using namespace MathUtils;
	
	/// 모든 데이터 type에 대해 어떻게 역직/직렬화 될지에 대해 정해줘야 함.
	

	class Property {
	public:
		explicit Property(const char* name) : m_Name(name) {}
		virtual ~Property() = default;

		const string& GetName() const { return m_Name; }

		//virtual void DrawEditor(Component* c) const;

		virtual void Serialize(Component* c, nlohmann::json& j) const = 0;
		virtual void DeSerialize(Component* c, const nlohmann::json& j) const = 0;

	private:
		std::string m_Name;
	};

	//////
	// 멤버 직접 접근 public 멤버만 가능 (아마 쓸일 없음)
	//template<typename T, typename Field>
	//class MemberProperty : public Property {
	//public:
	//	using MemberPtr = Field T::*;
	//
	//	MemberProperty(const char* name, MemberPtr member)
	//		: Property(name), m_Member(member) {
	//	}
	//
	//	//void DrawEditor(Component* c) const override {
	//	//	T* obj = static_cast<T*>(c);
	//	//	Editor::Draw(GetName().c_str(), obj->*m_Member);
	//	//}
	//
	//	void Serialize(Component* c, nlohmann::json& j) const override
	//	{
	//		T* obj = static_cast<T*>(c);
	//		j[GetName()] = obj->*m_Member;
	//	}
	//
	//	void DeSerialize(Component* c, const nlohmann::json& j) const override
	//	{
	//		if (!j.contains(GetName())) return;
	//		T* obj = static_cast<T*>(c);
	//		obj->*m_Member = j[GetName()].get<Field>();
	//	}
	//
	//private:
	//	MemberPtr m_Member;
	//};


	////
	//Getter, Setter 사용
	template<typename T, typename Value>
	class AccessorProperty : public Property {
	public:
		using Getter = const Value& (T::*)() const;
		using Setter = void (T::*)(const Value&);

		template<typename G, typename S>
		AccessorProperty(const char* name, G g, S s)
			: Property(name)
			, m_Get(reinterpret_cast<Getter>(g))
			, m_Set(reinterpret_cast<Setter>(s))
		{
		}

		void Serialize(Component* c, nlohmann::json& j) const override {
			T* obj = static_cast<T*>(c);
			Serializer<Value>::ToJson(j[GetName()], (obj->*m_Get)());
		}

		void DeSerialize(Component* c, const nlohmann::json& j) const override {
			if (!j.contains(GetName())) return;
			T* obj = static_cast<T*>(c);
			Value v{};
			Serializer<Value>::FromJson(j[GetName()], v);
			(obj->*m_Set)(v);
		}

	private:
		Getter m_Get;
		Setter m_Set;
	};



	using ComponentCreateFunc = function<unique_ptr<Component>()>;

	struct ComponentTypeInfo {
		string name;  // 컴포넌트 이름 TransformComponent
		ComponentCreateFunc factory;
		vector<unique_ptr<Property>> properties; // 컴포넌트가 가지고 있는 Property(변수 등)
	};

	class ComponentRegistry {
	public:
		static ComponentRegistry& Instance();

		void Register(ComponentTypeInfo* info);
		vector<string> GetTypeNames() const; // 등론된 이름 전체 return
		ComponentTypeInfo* Find(const string& name) {
			auto it = m_Types.find(name);
			return (it != m_Types.end()) ? it->second : nullptr;
		}


		void Check() {
			//개발 확인용
			for (const auto& [name, typeInfo] : m_Types) {
				std::cout << "Component: " << name << std::endl;

				if (!typeInfo) {
					std::cout << "  (TypeInfo is null)" << std::endl;
					continue;
				}

				for (const auto& prop : typeInfo->properties) {
					std::cout << "    Property: " << prop->GetName() << std::endl;
				}
			}
		}
	private:
		unordered_map<string, ComponentTypeInfo*> m_Types; //이름 : 컴포넌트
	};
	// 게임에서 정의된 컴포넌트 들과 프로퍼티 (사전) -> Component List

	
