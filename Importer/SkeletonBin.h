#pragma once

#pragma pack(push, 1)
struct SkelBinHeader
{
	uint32_t magic = 0x534B454C; // "SKEL"
	uint16_t version = 1;
	uint16_t boneCount = 0;
	uint32_t stringTableBytes = 0;
};

struct BoneBin
{
	uint32_t nameOffset = 0;
	int32_t  parentIndex = -1;
	float	 inverseBindPose[16]; // Row-Major
	float	 localBind[16];	      // aiNode local transform
};
#pragma pack(pop)

struct SkeletonBuildResult
{
	std::vector<BoneBin> bones;
	std::string stringTable;
	std::unordered_map<std::string, uint32_t> boneNameToIndex;
};

SkeletonBuildResult BuildSkeletonFromScene(const aiScene* scene);
bool ImportFBXToSkelBin(
	const std::string& fbxPath,
	const std::string& outSkelBin,
	std::unordered_map<std::string, uint32_t>& outBoneNameToIndex);