#pragma once
#include "Component.h"
#include "RenderData.h"
using namespace RenderData;

class LightComponent : public Component
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "LightComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	LightComponent();
	virtual ~LightComponent() = default;

	void SetType(RenderData::LightType type) { m_Type = type; }
	RenderData::LightType GetType() const    { return m_Type; }

	void  SetColor(const XMFLOAT3& color){ m_Color = color; }
	const XMFLOAT3& GetColor() const     { return m_Color;  }

	void  SetIntensity(float intensity){ m_Intensity = intensity; }
	float GetIntensity() const         { return m_Intensity;      }

	void  SetRange(float range){ m_Range = range; }
	float GetRange() const     { return m_Range;  }

	void  SetSpotAngle(float angle){ m_SpotAngle = angle; }
	float GetSpotAngle() const     { return m_SpotAngle;  }

	void  SetDirection(const XMFLOAT3& direction){ m_Direction = direction; }
	const XMFLOAT3& GetDirection() const         { return m_Direction;      }

	void  SetPosition(const XMFLOAT3& position){ m_Position = position; }
	const XMFLOAT3& GetPosition() const        { return m_Position;     }

	void  SetLightViewProj(const XMFLOAT4X4& viewProj){ m_LightViewProj = viewProj; }
	const XMFLOAT4X4& GetLightViewProj() const        { return m_LightViewProj;     }

	void SetCastShadow(bool castShadow) { m_CastShadow = castShadow; }
	bool CastShadow   () const          { return m_CastShadow;       }

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize  (nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

protected:
	LightType  m_Type = LightType::Directional;
	XMFLOAT3   m_Color        {};
	XMFLOAT3   m_Position	  {};
	FLOAT	   m_Intensity = 0.0f;
	FLOAT      m_Range     = 0.0f;
	FLOAT      m_SpotAngle = 0.0f;
	XMFLOAT3   m_Direction    {};
	XMFLOAT4X4 m_LightViewProj{};
	BOOL       m_CastShadow = true;
};

REGISTER_COMPONENT(LightComponent);

