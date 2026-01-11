#include "Component.h"
#include "GameObject.h"
#include "Reflection.h"

EventDispatcher& Component::GetEventDispatcher() const
{
    return m_Owner->GetEventDispatcher();
}

void Component::Serialize(nlohmann::json& j) const {
	auto* typeInfo = ComponentRegistry::Instance().Find(GetTypeName());
	if (!typeInfo) return;
	for (const auto& prop : typeInfo->properties) {
		prop->Serialize(const_cast<Component*>(this), j);
	}
}

void Component::Deserialize(const nlohmann::json& j) {

	auto* typeInfo = ComponentRegistry::Instance().Find(GetTypeName());
	if (!typeInfo) return;
	for (const auto& prop : typeInfo->properties) {
		prop->DeSerialize(this, j);
	}
}