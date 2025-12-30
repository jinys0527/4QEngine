#pragma once
#include "Component.h"
#include "RenderData.h"

class MaterialComponent : public Component
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "MaterialComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	MaterialComponent() = default;
	virtual ~MaterialComponent() = default;

	void SetMaterialHandle(MaterialHandle handle) { m_MaterialHandle = handle; }
	MaterialHandle GetMaterialHandle() const      { return m_MaterialHandle;   }

	void SetOverrides(const RenderData::MaterialData& overrides);
	void ClearOverrides();
	bool HasOverrides() const { return m_UseOverrides; }
	const RenderData::MaterialData& GetOverrides() const { return m_Overrides; }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

protected:
	MaterialHandle m_MaterialHandle;
	RenderData::MaterialData m_Overrides;
	bool m_UseOverrides = false;
};

REGISTER_COMPONENT(MaterialComponent);

