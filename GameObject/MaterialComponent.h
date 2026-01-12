#pragma once
#include "Component.h"
#include "RenderData.h"
#include "ResourceRefs.h"

class MaterialComponent : public Component
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "MaterialComponent";
	const char* GetTypeName() const override;

	MaterialComponent() = default;
	virtual ~MaterialComponent() = default;

	void				  SetMaterialHandle(const MaterialHandle& handle) { m_MaterialHandle = handle; }
	const MaterialHandle& GetMaterialHandle() const						  { return m_MaterialHandle;   }

	void LoadSetMaterial(const MaterialRef& materialRef)
	{
		m_Material = materialRef;
	}

	const MaterialRef& GetMaterial () const { return m_Material;  }


	void SetOverrides(const RenderData::MaterialData& overrides);
	void ClearOverrides();
	bool HasOverrides() const { return m_UseOverrides; }
	const RenderData::MaterialData& GetOverrides() const { return m_Overrides; }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	MaterialHandle			 m_MaterialHandle;
	MaterialRef				 m_Material;
	RenderData::MaterialData m_Overrides;
	bool					 m_UseOverrides = false;
};



