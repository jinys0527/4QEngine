#pragma once
#include "Component.h"
#include "TMHelper.h"
#include <vector>


enum class TransformPivotPreset
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight,
	Center
};

class TransformComponent : public Component, public IEventListener
{
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "TransformComponent";
	const char* GetTypeName() const override { return StaticTypeName; }

	using Vec2F = Math::Vector2F;
	using Matrix3X2F = D2D1::Matrix3x2F;

	TransformComponent() : Component(), m_Position{ 0, 0 }, m_Rotation(0.0f), m_Scale{ 1.0f, 1.0f }, m_IsDirty(false), m_Parent(nullptr)
	{
		m_LocalMatrix = D2D1::Matrix3x2F::Identity();
		m_WorldMatrix = D2D1::Matrix3x2F::Identity();
	}

	virtual ~TransformComponent()
	{
		for (auto* child : m_Children)
		{
			child->m_Parent = nullptr;
		}
	}

	TransformComponent* GetParent() const { return m_Parent; }

	void SetParent(TransformComponent* newParent);
	void DetachFromParent();

	void AddChild(TransformComponent* child);
	void RemoveChild(TransformComponent* child);

	void SetPosition(const Vec2F& pos) { m_Position = pos; SetDirty(); }
	void SetRotation(const float deg) { m_Rotation = deg; SetDirty(); }
	void SetScale(const Vec2F& scale) { m_Scale = scale; SetDirty(); }
	void SetZOrder(const int zorder) { m_ZOrder = zorder; }

	int GetZOrder() { return m_ZOrder; }

	const Vec2F& GetPosition() const { return m_Position; }
	float GetRotation() const { return m_Rotation; }
	const Vec2F& GetScale() const { return m_Scale; }

	void Translate(const Vec2F& delta);
	void Translate(const float x, const float y);

	void Rotate(float deg);

	Vec2F GetForward() const;

	const Matrix3X2F& GetWorldMatrix();
	const Matrix3X2F& GetLocalMatrix();

	Matrix3X2F GetInverseWorldMatrix();

	void SetPivotPreset(TransformPivotPreset preset, const D2D1_SIZE_F& size);
	D2D1_POINT_2F GetPivotPoint() const { return m_Pivot; }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	std::vector<TransformComponent*>& GetChildrens()
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
	Vec2F m_Position = { 0.0f, 0.0f };
	float m_Rotation = 0.0f;
	Vec2F m_Scale = { 1.0f, 1.0f };

	D2D1_POINT_2F m_Pivot = { 0.0f, 0.0f };

	TransformComponent* m_Parent = nullptr;
	std::vector<TransformComponent*> m_Children;

	Matrix3X2F m_LocalMatrix;
	Matrix3X2F m_WorldMatrix;

	bool m_IsWPressed = false;
	bool m_IsAPressed = false;
	bool m_IsSPressed = false;
	bool m_IsDPressed = false;

	bool m_IsDirty = true;

	int m_ZOrder = 0;
};

REGISTER_COMPONENT(TransformComponent);