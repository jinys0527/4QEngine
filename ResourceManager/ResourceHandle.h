#pragma once

#include <cstdint>

// id + generation만 들고 있는 가벼운 참조값

struct ResourceHandle
{
	uint32_t id = 0;
	uint32_t generation = 0;

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
	uint32_t id = 0;
	uint32_t generation = 0;

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

using MeshHandle     = TypedHandle<MeshTag>;
using MaterialHandle = TypedHandle<MaterialTag>;
using TextureHandle  = TypedHandle<TextureTag>;