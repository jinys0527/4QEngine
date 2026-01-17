#pragma once

#include <array>
#include <cstdlib>
#include <string>
#include <vector>
#include <unordered_map>

#include <DirectXMath.h>
#include "ResourceHandle.h"
#include <Windows.h>

namespace RenderData
{
	using DirectX::XMFLOAT2;
	using DirectX::XMFLOAT3;
	using DirectX::XMFLOAT4;
	using DirectX::XMFLOAT4X4;

	struct Vertex 
	{
		XMFLOAT3 position{ 0.0f, 0.0f, 0.0f };
		XMFLOAT3 normal  { 0.0f, 0.0f, 0.0f };
		XMFLOAT2 uv      { 0.0f, 0.0f };
		XMFLOAT4 tangent { 0.0f, 0.0f, 0.0f, 1.0f };
		std::array<uint16_t, 4> boneIndices{ 0, 0, 0, 0 };
		std::array<float, 4>    boneWeights{ 0, 0, 0, 0 }; // normalized to 0 ~ 1
	};

	struct Vertex_PU
	{
		XMFLOAT3 pos	{ 0.0f, 0.0f, 0.0f };
		XMFLOAT2 uv		{ 0.0f, 0.0f };
	};
	
	struct MeshData
	{
		struct SubMesh
		{
			UINT32 indexStart = 0;
			UINT32 indexCount = 0;
			MaterialHandle material = MaterialHandle::Invalid();
			std::string name;
			XMFLOAT4X4 localToWorld;
		};

		std::vector<Vertex>   vertices;
		std::vector<UINT32>   indices;
		std::vector<SubMesh>  subMeshes;
		BOOL hasSkinning = false;
	};

	enum class MaterialTextureSlot : uint8_t
	{
		Albedo = 0,
		Normal,
		Metallic,
		Roughness,
		AO,
		Env,
		TEX_MAX
	};


	struct MaterialData
	{
		XMFLOAT4 baseColor{ 1.0f, 1.0f, 1.0f, 1.0f };
		FLOAT    metallic = 0.0f;
		FLOAT    roughness = 1.0f;
		FLOAT    padding[2]{ 0.0f, 0.0f };
		std::array<TextureHandle, static_cast<size_t>(MaterialTextureSlot::TEX_MAX)> textures{};
		ShaderAssetHandle  shaderAsset  = ShaderAssetHandle::Invalid();
		VertexShaderHandle vertexShader = VertexShaderHandle::Invalid();
		PixelShaderHandle  pixelShader  = PixelShaderHandle::Invalid();
	};

	struct TextureData
	{
		std::string path;
		BOOL sRGB = true;
	};

	struct VertexShaderData
	{
		std::string path;
	};

	struct PixelShaderData
	{
		std::string path;
	};

	struct ShaderAssetData
	{
		VertexShaderHandle vertexShader = VertexShaderHandle::Invalid();
		PixelShaderHandle  pixelShader  = PixelShaderHandle::Invalid();
	};

	enum class LightType : uint8_t
	{
		Directional,
		Point,
		Spot,
		LIGHT_MAX
	};

	struct LightData
	{
		LightType  type = LightType::Directional;
		XMFLOAT3   posiiton{ 0.0f, 0.0f, 0.0f };
		FLOAT      range = 0.0f;
		XMFLOAT3   direction{ 0.0f, -1.0f, 0.0f };
		FLOAT      spotAngle = 0.0f;
		XMFLOAT3   color{ 1.0f, 1.0f, 1.0f };
		FLOAT      intensity = 1.0f;
		XMFLOAT4X4 lightViewProj{};
		BOOL       castShadow = true;
		FLOAT      padding[3];
	};

	struct Bone
	{
		std::string name;
		INT32       parentIndex = -1;
		XMFLOAT4X4  bindPose{};
		XMFLOAT4X4  inverseBindPose{};
	};

	struct Skeleton
	{
		std::vector<Bone>			bones;
		std::vector<int>			upperBodyBones;
		std::vector<int>			lowerBodyBones;
	};

	struct AnimationKeyFrame
	{
		FLOAT time = 0.0f;
		XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
		XMFLOAT4 rotation   { 0.0f, 0.0f, 0.0f, 0.0f };
		XMFLOAT3 scale	    { 1.0f,1.0f,1.0f };
	};

	struct AnimationTrack
	{
		INT32                          boneIndex = -1;
		std::vector<AnimationKeyFrame> keyFrames;
	};

	struct AnimationClip
	{
		std::string name;
		FLOAT duration = 0.0f;
		FLOAT ticksPerSecond = 0.0f;
		std::vector<AnimationTrack> tracks;
	};

	struct CameraContext
	{
		UINT32        width = 0;
		UINT32        height = 0;
		XMFLOAT4X4    view{};
		XMFLOAT4X4    proj{};
		XMFLOAT4X4    viewProj{};
		XMFLOAT3      cameraPos{ 0.0f, 0.0f,0.0f };
		FLOAT         exposure = 1.0f;
		XMFLOAT3      ambientColor{ 0.0f,0.0f,0.0f };
	};

	struct FrameContext
	{
		UINT32       frameIndex = 0;
		FLOAT        deltaTime = 0.0f;
		CameraContext editorCamera{};
		CameraContext gameCamera{};
		FLOAT        padding = 0.0f;
	};

	struct RenderItem
	{
		MeshHandle     mesh     = MeshHandle::Invalid();
		MaterialHandle material = MaterialHandle::Invalid();
		SkeletonHandle skeleton = SkeletonHandle::Invalid();
		XMFLOAT4X4     world{};
		UINT64         sortKey = 0;
		UINT32		   skinningPaletteOffset = 0;
		UINT32		   skinningPaletteCount  = 0;
		UINT32		   indexStart = 0;
		UINT32		   indexCount = 0;
		bool		   useSubMesh = false;
		XMFLOAT4X4     localToWorld{};
	};

	enum RenderLayer
	{
		OpaqueItems,
		TransparentItems,
		UIItems,
		Layer_MAX_
	};

	struct FrameData
	{
		FrameContext            context;
		std::unordered_map<RenderLayer, std::vector<RenderItem>> renderItems;
		std::vector<LightData>  lights;
		std::vector<XMFLOAT4X4> skinningPalettes;
	};
}