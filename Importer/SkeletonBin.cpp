#include "pch.h"
#include "SkeletonBin.h"
#include "BinHelper.h"

static void MatToRowMajor16(const aiMatrix4x4& m, float out16[16])
{
	// transpose (Assimp -> DX)
	out16[0]  = m.a1; out16[1]  = m.b1; out16[2]  = m.c1; out16[3]  = m.d1;
	out16[4]  = m.a2; out16[5]  = m.b2; out16[6]  = m.c2; out16[7]  = m.d2;
	out16[8]  = m.a3; out16[9]  = m.b3; out16[10] = m.c3; out16[11] = m.d3;
	out16[12] = m.a4; out16[13] = m.b4; out16[14] = m.c4; out16[15] = m.d4;
}

#ifdef _DEBUG
static std::string ReadStringAtOffset(const std::string& table, uint32_t offset)
{
	if (offset >= table.size())
		return {};
	return std::string(&table[offset]);
}

static void WriteSkeletonDebug(const std::string& outSkelBin, const SkeletonBuildResult& skel)
{
	std::ofstream ofs(outSkelBin + ".debug.txt");
	if (!ofs)
		return;

	ofs << "boneCount=" << skel.bones.size() << "\n";
	for (size_t i = 0; i < skel.bones.size(); ++i)
	{
		const BoneBin& bone = skel.bones[i];
		ofs << "bone[" << i << "] name=" << ReadStringAtOffset(skel.stringTable, bone.nameOffset)
			<< " parentIndex=" << bone.parentIndex << "\n";
		ofs << "  localBind=";
		for (int m = 0; m < 16; ++m)
		{
			ofs << bone.localBind[m];
			ofs << (m == 15 ? "\n" : ",");
		}
		ofs << "  inverseBindPose=";
		for (int m = 0; m < 16; ++m)
		{
			ofs << bone.inverseBindPose[m];
			ofs << (m == 15 ? "\n" : ",");
		}
	}
}
#endif


static aiMatrix4x4 Identity()
{
	aiMatrix4x4 I;
	I.a1 = 1; I.a2 = 0; I.a3 = 0; I.a4 = 0;
	I.b1 = 0; I.b2 = 1; I.b3 = 0; I.b4 = 0;
	I.c1 = 0; I.c2 = 0; I.c3 = 1; I.c4 = 0;
	I.d1 = 0; I.d2 = 0; I.d3 = 0; I.d4 = 1;
	return I;
}

static void CollectUsedBoneNames(const aiScene* scene, std::unordered_set<std::string>& out)
{
	out.clear();
	if (!scene || !scene->HasMeshes()) return;

	for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];
		if (!mesh || !mesh->HasBones()) continue;

		for (uint32_t b = 0; b < mesh->mNumBones; ++b)
		{
			const aiBone* bone = mesh->mBones[b];
			if (!bone) continue;
			out.insert(bone->mName.C_Str());
		}
	}
}

static bool IsUsedBoneNode(const aiNode* node, const std::unordered_set<std::string>& usedBoneName)
{
	if (!node) return false;
	return usedBoneName.find(node->mName.C_Str()) != usedBoneName.end();
}

static int32_t FindParentBoneIndex(const aiNode* node, const std::unordered_map<std::string, uint32_t>& boneNameToIndex)
{
	// node의 부모로 올라가면서 가장 가까운 bone 노드 찾기
	const aiNode* p = node ? node->mParent : nullptr;
	while (p)
	{
		auto it = boneNameToIndex.find(p->mName.C_Str());
		if (it != boneNameToIndex.end())
			return (int32_t)it->second;
		p = p->mParent;
	}
	return -1;
}

static void FillParentIndicesFromNodes(const aiNode* node, SkeletonBuildResult& skel)
{
	if (!node) return;

	auto it = skel.boneNameToIndex.find(node->mName.C_Str());
	if (it != skel.boneNameToIndex.end())
	{
		const uint32_t idx = it->second;
		skel.bones[idx].parentIndex = FindParentBoneIndex(node, skel.boneNameToIndex);
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i)
		FillParentIndicesFromNodes(node->mChildren[i], skel);
}

static void TraverseAndRegisterBones(const aiNode* node,
	const std::unordered_set<std::string>& usedBoneNames,
	SkeletonBuildResult& outSkel)
{
	if (!node) return;

	// 전위 순회
	if (IsUsedBoneNode(node, usedBoneNames))
	{
		const std::string name = node->mName.C_Str();

		// 중복 방지
		if (outSkel.boneNameToIndex.find(name) == outSkel.boneNameToIndex.end())
		{
			BoneBin bb{}; 

			bb.nameOffset = AddString(outSkel.stringTable, name);
			bb.parentIndex = -1;
			MatToRowMajor16(node->mTransformation, bb.localBind);

			// inverseBindPose : aiBone Offset 매칭으로 채움
			aiMatrix4x4 I = Identity();
			MatToRowMajor16(I, bb.inverseBindPose);

			const uint32_t idx = (uint32_t)outSkel.bones.size();
			outSkel.bones.push_back(bb);
			outSkel.boneNameToIndex.emplace(name, idx);
		}
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i)
		TraverseAndRegisterBones(node->mChildren[i], usedBoneNames, outSkel);
}


static void FillInverseBindPosesFromMeshes(const aiScene* scene, SkeletonBuildResult& skel)
{
	if (!scene || !scene->HasMeshes()) return;

	for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];
		if (!mesh || !mesh->HasBones()) continue;

		for (uint32_t b = 0; b < mesh->mNumBones; ++b)
		{
			const aiBone* bone = mesh->mBones[b];
			if (!bone) continue;

			const std::string name = bone->mName.C_Str();
			auto it = skel.boneNameToIndex.find(name);
			if (it == skel.boneNameToIndex.end())
				continue;		//usedBoneNames에 없거나 node에 없는 케이스

			const uint32_t idx = it->second;

			// aiNode : mOffsetMatrix - inverseBindPose
			MatToRowMajor16(bone->mOffsetMatrix, skel.bones[idx].inverseBindPose);
		}
	}
}


SkeletonBuildResult BuildSkeletonFromScene(const aiScene* scene)
{
	SkeletonBuildResult out;
	if (!scene || !scene->mRootNode) return out;
	
	std::unordered_set<std::string> usedBoneNames;
	CollectUsedBoneNames(scene, usedBoneNames);

	// bone이 하나도 없으면 빈 Skeleton 반환(정적 에셋)
	if (usedBoneNames.empty())
		return out;

	// 1) aiNode 트리에서 used bone 노드만 등록(순서 고정)
	TraverseAndRegisterBones(scene->mRootNode, usedBoneNames, out);

	// 2) parentIndex 채우기(노드 기반)
	FillParentIndicesFromNodes(scene->mRootNode, out);

	// 3) inverseBindPose를 aiBone offset으로 채우기
	FillInverseBindPosesFromMeshes(scene, out);

	return out;
}

bool ImportFBXToSkelBin(
	const aiScene* scene,
	const std::string& outSkelBin, 
	std::unordered_map<std::string, uint32_t>& outBoneNameToIndex,
	nlohmann::json& skeletonMeta)
{
	if (!scene || !scene->mRootNode) return false;

	SkeletonBuildResult skel = BuildSkeletonFromScene(scene);

	// bone이 없는 정적 FBX면 skelbin을 안 만들지/빈 파일을 만들지 정책 필요
	// 여기서는 "bone 없으면 false" 대신 "빈 스켈레톤 파일 생성"으로 처리 가능
	// 일단: bone 없으면 outBoneNameToIndex 비우고 true 반환(정적 에셋 케이스)
	outBoneNameToIndex = skel.boneNameToIndex;

#ifdef _DEBUG
	WriteSkeletonDebug(outSkelBin, skel);
#endif

	if (skel.bones.empty())
		return true;

	SkelBinHeader header{};
	header.version          = 2;
	header.boneCount        = (uint16_t)skel.bones.size();
	header.stringTableBytes = (uint32_t)skel.stringTable.size();
	std::vector<int32_t>		   upperBodyIndices;
	std::vector<int32_t>		   lowerBodyIndices;

	const auto fillIndices = [](const nlohmann::json& arr, std::vector<int32_t>& out)
		{
			out.clear();
			if (!arr.is_array())
				return;

			out.reserve(arr.size());
			for (const auto& entry : arr)
			{
				if (entry.is_number_integer())
				{
					out.push_back(entry.get<int32_t>());
				}
			}
		};

	const auto addIndexIfExists = [&skel](const std::string& name, std::vector<int32_t>& out)
		{
			auto it = skel.boneNameToIndex.find(name);
			if (it != skel.boneNameToIndex.end())
			{
				out.push_back(static_cast<int32_t>(it->second));
			}
		};

	const auto autoFillFromUnrealNames = [&]()
		{
			const std::vector<std::string> upperNames = {
				// Spine / Upper body
				"spine_01", "spine_02", "spine_03",

				// Arms
				"clavicle_l", "upperarm_l", "lowerarm_l", "hand_l",
				"clavicle_r", "upperarm_r", "lowerarm_r", "hand_r",

				// Neck / Head
				"neck_01", "head",

				// Fingers (Left)
				"thumb_01_l",  "thumb_02_l",  "thumb_03_l",
				"index_01_l",  "index_02_l",  "index_03_l",
				"middle_01_l", "middle_02_l", "middle_03_l",
				"ring_01_l",   "ring_02_l",   "ring_03_l",
				"pinky_01_l",  "pinky_02_l",  "pinky_03_l",

				// Fingers (Right)
				"thumb_01_r",  "thumb_02_r",  "thumb_03_r",
				"index_01_r",  "index_02_r",  "index_03_r",
				"middle_01_r", "middle_02_r", "middle_03_r",
				"ring_01_r",   "ring_02_r",   "ring_03_r",
				"pinky_01_r",  "pinky_02_r",  "pinky_03_r",

				// Arm twist (optional but recommended)
				"upperarm_twist_01_l", "lowerarm_twist_01_l",
				"upperarm_twist_01_r", "lowerarm_twist_01_r"
			};

			const std::vector<std::string> lowerNames = {
				// Root / Pelvis
				"root",        // 스켈레톤에 존재한다면 포함 권장
				"pelvis",

				// Legs
				"thigh_l", "calf_l", "foot_l", "ball_l",
				"thigh_r", "calf_r", "foot_r", "ball_r",

				// Leg twist (optional but recommended)
				"thigh_twist_01_l", "calf_twist_01_l",
				"thigh_twist_01_r", "calf_twist_01_r"
			};

			for (const auto& name : upperNames)
			{
				addIndexIfExists(name, upperBodyIndices);
			}

			for (const auto& name : lowerNames)
			{
				addIndexIfExists(name, lowerBodyIndices);
			}

			skeletonMeta["upperBodyBones"] = upperBodyIndices;
			skeletonMeta["lowerBodyBones"] = lowerBodyIndices;
		};

	if (skeletonMeta.is_object() && !skeletonMeta.empty())
	{
		fillIndices(skeletonMeta.value("upperBodyBones", nlohmann::json::array()), upperBodyIndices);
		fillIndices(skeletonMeta.value("lowerBodyBones", nlohmann::json::array()), lowerBodyIndices);
	}
	else
	{
		autoFillFromUnrealNames();
	}

	header.upperCount          = static_cast<uint32_t>(upperBodyIndices.size());
	header.lowerCount          = static_cast<uint32_t>(lowerBodyIndices.size());


	std::ofstream ofs(outSkelBin, std::ios::binary);
	if (!ofs) return false;

	ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
	if (!skel.bones.empty())
		ofs.write(reinterpret_cast<const char*>(skel.bones.data()), sizeof(BoneBin) * skel.bones.size());
	if (!skel.stringTable.empty())
		ofs.write(reinterpret_cast<const char*>(skel.stringTable.data()), skel.stringTable.size());
	if (!upperBodyIndices.empty())
		ofs.write(reinterpret_cast<const char*>(upperBodyIndices.data()), sizeof(int32_t) * upperBodyIndices.size());
	if (!lowerBodyIndices.empty())
		ofs.write(reinterpret_cast<const char*>(lowerBodyIndices.data()), sizeof(int32_t) * lowerBodyIndices.size());

	return true;
}
