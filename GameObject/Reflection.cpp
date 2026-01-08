#include "Reflection.h"



ComponentRegistry& ComponentRegistry::Instance()
{
    static ComponentRegistry instance;
    return instance;
}

void ComponentRegistry::Register(ComponentTypeInfo* info)
{
    m_Types[info->name] = info;
}
