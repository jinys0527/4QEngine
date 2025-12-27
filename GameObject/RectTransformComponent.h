#pragma once
#include "Component.h"
#include "SimpleMathHelper.h"

enum class AnchorPrset
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	Center,
	FullStretch
};

enum class RectTransformPivotPreset
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	Center
};

struct Anchor
{
	D2D1_POINT_2F minPoint = { 0.0f, 0.0f };		// 왼쪽 하단		(0.0 ~ 1.0)
	D2D1_POINT_2F maxPoint = { 0.0f, 0.0f };		// 오른쪽 상단	(0.0 ~ 1.0)
};

class RectTransformComponent : public Component
{
	using Vec2F = Math::Vector2F;
	using Mat3X2F = D2D1::Matrix3x2F;

public:
	static constexpr const char* StaticTypeName = "RectTransformComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	RectTransformComponent() = default;
	RectTransformComponent(const Vec2F& position,
		const Vec2F& size,
		AnchorPrset anchor = AnchorPrset::TopLeft,
		RectTransformPivotPreset pivot = RectTransformPivotPreset::Center,
		float rotation = 0.0f,
		const Vec2F& scale = { 1.0f, 1.0f }) { }
	virtual ~RectTransformComponent() = default;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void SetParent(RectTransformComponent* newParent);
	void DetachFromParent();

	void AddChild(RectTransformComponent* child);
	void RemoveChild(RectTransformComponent* child);

	Mat3X2F GetInverseWorldMatrix();

	void SetAnchorPreset(AnchorPrset preset);
	Anchor GetAnchor() const { return m_Anchor; }

	void SetPivotPreset(RectTransformPivotPreset preset);

	void SetPosition(const Vec2F& position)
	{
		m_Position.x = position.x + 960.0f - (m_Size.x / 2.0f);
		m_Position.y = position.y + 540.0f - (m_Size.y / 2.0f);
	}

	Vec2F GetPosition() const 
	{
		return m_Position;
	}

	void SetSize(const Vec2F& size)
	{
		m_Size = size;
	}

	const Vec2F& GetSize() const
	{
		return m_Size;
	}

	void SetRotation(float rotation)
	{
		m_Rotation = rotation;
	}

	float GetRotation() const
	{
		return m_Rotation;
	}

	void SetScale(const Vec2F& scale)
	{
		m_Scale = scale;
	}

	Vec2F GetScale() const
	{
		return m_Scale;
	}

	D2D1_POINT_2F GetPivot() const
	{
		return m_Pivot;
	}

	const Mat3X2F& GetWorldMatrix();
	const Mat3X2F& GetLocalMatrix();

	RectTransformComponent* GetParent()
	{
		return m_Parent;
	}

	std::vector<RectTransformComponent*> GetChild()
	{
		return m_Children;
	}

private:
	void SetDirty()
	{
		m_IsDirty = true;
		for (auto* child : m_Children)
		{
			child->SetDirty();
		}
	}

	void UpdateMatrices();

private:
	Vec2F m_Position;
	Anchor m_Anchor;
	D2D1_POINT_2F m_Pivot = { 0.5f, 0.5f };			// 0.0 ~ 1.0
	Vec2F m_Size;
	float m_Rotation;
	Vec2F m_Scale;

	Mat3X2F m_LocalMatrix;
	Mat3X2F m_WorldMatrix;

	RectTransformComponent* m_Parent = nullptr;
	std::vector<RectTransformComponent*> m_Children;

	bool m_IsDirty = true;
};

REGISTER_COMPONENT(RectTransformComponent);