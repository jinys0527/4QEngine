#pragma once
#include "Component.h"
#include "RenderData.h"

class TransformComponent;

class MeshRenderer : public Component
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "MeshRenderer";
	const char* GetTypeName() const override;
	
	MeshRenderer() = default;
	virtual ~MeshRenderer() = default;

	void SetMeshHandle(MeshHandle handle) { m_MeshHandle = handle; }
	MeshHandle GetMeshHandle() const      { return m_MeshHandle;   }

	void SetMaterialHandle(MaterialHandle handle) { m_MaterialHandle = handle; }
	MaterialHandle GetMaterialHandle() const      { return m_MaterialHandle;   }

	void SetVisible(bool visible) { m_Visible = visible; }
	bool IsVisible () const       { return m_Visible;    }
	void SetCastShadow(bool castShadow) { m_CastShadow = castShadow; }
	bool CastShadow   () const          { return m_CastShadow;       }
	void SetReceiveShadow(bool receiveShadow) { m_ReceiveShadow = receiveShadow; }
	bool ReceiveShadow   () const             { return m_ReceiveShadow;          }

	void  SetRenderLayer(UINT8 layer) { m_RenderLayer = layer; }
	UINT8 GetRenderLayer() const      { return m_RenderLayer;  }

	virtual bool BuildRenderItem(RenderData::RenderItem& out) const;

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize  (nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

private:
	void ResolveHandles       (MeshHandle& mesh, MaterialHandle& material) const;

protected:
	static UINT64 BuildSortKey(MeshHandle mesh, MaterialHandle material, UINT8 layer);
	TransformComponent* GetTransform() const;

	MeshHandle     m_MeshHandle      = MeshHandle::Invalid();
	MaterialHandle m_MaterialHandle  = MaterialHandle::Invalid();
	bool  m_Visible                  = true;
	bool  m_CastShadow               = true;
	bool  m_ReceiveShadow            = true;
	UINT8 m_RenderLayer              = 0;
};

