#pragma once
//#include <type_traits>
#include "Reflection.h"
//매크로만 등록 하는 곳
// *** 매크로는 단순 문자 치환으로 띄어쓰기 줄바꿈에 매우 민감. 겨우 띄어쓰기 하나땜에 안될 수 있음




// symbol link용 함수 등록 매크로
#define REGISTER_LINK(TYPE)\
    extern "C" void Link_##TYPE();\
    extern "C" void Link_##TYPE(){}

//실제 컴포넌트 등록 
#define REGISTER_COMPONENT_DATA(TYPE) \
    static ComponentTypeInfo TYPE##_TypeInfo = { \
        #TYPE, [](){ return std::make_unique<TYPE>(); }, {} \
    }; \
    const char* TYPE::GetTypeName() const { return #TYPE; } \
    static bool TYPE##_Registered = [](){ \
		std::cout << "Registering component: " << #TYPE << std::endl;\
        ComponentFactory::Instance().Register(#TYPE, [](){ return std::make_unique<TYPE>(); }); \
        ComponentRegistry::Instance().Register(&TYPE##_TypeInfo); \
        return true; \
    }();

// Link함수 + Data 자동화
#define REGISTER_COMPONENT(TYPE) \
     REGISTER_COMPONENT_DATA(TYPE) \
     REGISTER_LINK(TYPE)





// private 어캐 함? Get
// float3  어캐 할 거냐
// parent
//#define REGISTER_PROPERTY(TYPE, FIELD) \
//    static bool TYPE##_##FIELD##_registered = [](){ \
//    TYPE##_TypeInfo.properties.emplace_back( \
//    std::make_unique<MemberProperty<TYPE, decltype(TYPE::FIELD)>>( \
//    #FIELD, &TYPE::FIELD \
//	)); \
//    return true; \
//    }();


#define REGISTER_PROPERTY(TYPE, NAME)\
    static bool TYPE##_##NAME##_registered = [](){\
    using RawReturn = decltype(std::declval<TYPE>().Get##NAME());\
    using ValueType = std::remove_cvref_t<RawReturn>; \
    TYPE##_TypeInfo.properties.emplace_back(\
    std::make_unique<AccessorProperty<TYPE, ValueType>>(\
    #NAME, &TYPE::Get##NAME, &TYPE::Set##NAME\
    )\
    );\
    return true;\
    }();