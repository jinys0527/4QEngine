#pragma once

enum class ETextureType : uint8_t { ALBEDO, NORMAL, METALLIC, ROUGHNESS, AO, EMISSIVE, MAX };
static constexpr size_t TEX_MAX = static_cast<size_t>(ETextureType::MAX);

#pragma pack(push, 1)
struct MatBinHeader
{
	uint32_t magic = 0x4D41544C; // "MATL"
	uint16_t version = 1;
	uint16_t materialCount = 0;
	uint32_t stringTableBytes = 0;
};

struct MatData
{
	uint32_t materialNameOffset = 0;   // string table offset
	float baseColor[4];
	float metallic = 0.0f;
	float roughness = 1.0f;
	uint8_t alphaMode = 0;			   // 0 : Opaque 1 : Mask 2 : Blend
	float alphaCutOff = 0.5f;
	float opacity = 1.0f;
	uint8_t doubleSided = 0;
	uint8_t pad[3] = {0, 0, 0};
	uint32_t  texPathOffset[TEX_MAX] = {};	// 0이면 없음
};
#pragma pack(pop)

bool ImportFBXToMaterialBin(const std::string& fbxPath, const std::string& outMaterialBin);
