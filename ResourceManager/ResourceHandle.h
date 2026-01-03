#pragma once

#include <cstdint>
#include <Windows.h>

// id + generation만 들고 있는 가벼운 참조값

struct ResourceHandle
{
	UINT32 id = 0;
	UINT32 generation = 0;

	bool IsValid() const
	{
		return id != 0;
	}

	static ResourceHandle Invalid()
	{
		return {};
	}

	bool operator==(const ResourceHandle& other) const
	{
		return id == other.id && generation == other.generation;
	}

	bool operator!=(const ResourceHandle& other) const
	{
		return !(*this == other);
	}
};

template <typename Tag>
struct TypedHandle
{
	UINT32 id = 0;
	UINT32 generation = 0;

	bool IsValid() const
	{
		return id != 0;
	}

	static TypedHandle Invalid()
	{
		return {};
	}

	bool operator==(const TypedHandle& other) const
	{
		return id == other.id && generation == other.generation;
	}

	bool operator!=(const TypedHandle& other) const
	{
		return !(*this == other);
	}
};

struct MeshTag
{
};

struct MaterialTag
{
};

struct TextureTag
{
};

struct ShaderTag
{
};

struct SkeletonTag
{
};

struct AnimationTag
{
};

using MeshHandle      = TypedHandle<MeshTag>;
using MaterialHandle  = TypedHandle<MaterialTag>;
using TextureHandle   = TypedHandle<TextureTag>;
using ShaderHandle    = TypedHandle<ShaderTag>;
using SkeletonHandle  = TypedHandle<SkeletonTag>;
using AnimationHandle = TypedHandle<AnimationTag>;