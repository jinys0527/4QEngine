#pragma once
#include <string>
#include <windows.h>

struct MeshRef
{
	std::string assetPath;
	UINT32      assetIndex = 0;
};

struct MaterialRef
{
	std::string assetPath;
	UINT32      assetIndex = 0;
};

struct TextureRef
{
	std::string assetPath;
	UINT32      assetIndex = 0;
};

struct SkeletonRef
{
	std::string assetPath;
	UINT32      assetIndex = 0;
};

struct AnimationRef
{
	std::string assetPath;
	UINT32      assetIndex = 0;
};