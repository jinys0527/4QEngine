#pragma once
#include "UIComponent.h"
#include "ResourceHandle.h"

class UIProgressBarComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIProgressBarComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetPercent(const float& percent);
	const float& GetPercent() const { return m_Percent; }

	void SetBackgroundMaterialHandle(const MaterialHandle& handle) { m_BackgroundMaterial = handle; }
	const MaterialHandle& GetBackgroundMaterialHandle() const { return m_BackgroundMaterial; }

	void SetFillMaterialHandle(const MaterialHandle& handle) { m_FillMaterial = handle; }
	const MaterialHandle& GetFillMaterialHandle() const { return m_FillMaterial; }

private:
	float m_Percent = 0.0f;
	MaterialHandle m_BackgroundMaterial = MaterialHandle::Invalid();
	MaterialHandle m_FillMaterial = MaterialHandle::Invalid();
};

