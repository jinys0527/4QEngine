#pragma once

#include <imgui.h>
// #include "RenderTargetContext.h" -> 필요 

class EditorViewport
{
public:
	EditorViewport() = default;
	~EditorViewport() = default;

	//bool Draw(const RenderTargetContext& renderTarget); 
	const ImVec2& GetViewportSize() const { return m_ViewportSize; }

private:
	ImVec2 m_ViewportSize{ 0.0f, 0.0f };
};