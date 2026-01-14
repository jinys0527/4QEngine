#pragma once

#include <imgui.h>
#include <string>
#include "RenderTargetContext.h"

class EditorViewport
{
public:
	//EditorViewport() = default;
	explicit EditorViewport(std::string windowName = "Editor"): m_WindowName(std::move(windowName)){}
	~EditorViewport() = default;

	bool Draw(const RenderTargetContext& renderTarget); 
	const ImVec2& GetViewportSize() const { return m_ViewportSize;}

	bool IsHovered() const { return m_IsHovered; }
private:
	std::string m_WindowName; 
	ImVec2 m_ViewportSize{ 0.0f, 0.0f };
	bool m_IsHovered = false;
};