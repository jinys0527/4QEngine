#include "pch.h"
#include "EditorViewport.h"

bool EditorViewport::Draw(const RenderTargetContext& renderTarget)
{
	ImGui::Begin("Scene View");

	ImVec2 available = ImGui::GetContentRegionAvail();
    ImVec2 clamped = ImVec2((std::max)(1.0f, available.x), (std::max)(1.0f, available.y));

	const bool sizeChanged = (static_cast<int>(clamped.x) != static_cast<int>(m_ViewportSize.x))
		|| (static_cast<int>(clamped.y) != static_cast<int>(m_ViewportSize.y));
	m_ViewportSize = clamped;

	if (renderTarget.GetShaderResourceView())
	{
		ImGui::Image(reinterpret_cast<ImTextureID>(renderTarget.GetShaderResourceView()), clamped, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
	}
	else
	{
		ImGui::Text("Scene render target is not ready.");
	}

	ImGui::End();

	return sizeChanged;
}