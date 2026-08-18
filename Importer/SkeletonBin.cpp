#include "pch.h"
#include "SkeletonBin.h"
#include "BinHelper.h"

static void MatToRowMajor16(const aiMatrix4x4& m, float out16[16])
{
	// transpose (Assimp -> DX)
	out16[0] = m.a1; out16[1] = m.b1; out16[2] = m.c1; out16[3] = m.d1;
	out16[4] = m.a2; out16[5] = m.b2; out16[6] = m.c2; out16[7] = m.d2;
	out16[8] = m.a3; out16[9] = m.b3; out16[10] = m.c3; out16[11] = m.d3;
	out16[12] = m.a4; out16[13] = m.b4; out16[14] = m.c4; out16[15] = m.d4;
}

static std::string ReadStringAtOffset(const std::string& table, uint32_t offset)
{
	if (offset >= table.size())
		return {};
	return std::string(&table[offset]);
}

static const aiNode* FindNodeByName(const aiNode* node, const std::string& name)
{
	if (!node)
		return nullptr;

	if (name == node->mName.C_Str())
		return node;

	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	{
		if (const aiNode* found = FindNodeByName(node->mChildren[i], name))
		{
			return found;
		}
	}

	return nullptr;
}

static aiMatrix4x4 GetGlobalTransform(const aiNode* node)
{
	aiMatrix4x4 global = node ? node->mTransformation : aiMatrix4x4();
	for (const aiNode* parent = node ? node->mParent : nullptr; parent; parent = parent->mParent)
	{
		global = parent->mTransformation * global;
	}
	return global;
}

#ifdef _DEBUG

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

static void CollectRequiredBoneNames(
	const aiNode* node,
	const std::unordered_set<std::string>& usedBoneNames,
	std::unordered_set<std::string>& outRequired,
	std::vector<std::string>& ancestors)
{
	if (!node) return;

	const std::string name = node->mName.C_Str();
	ancestors.push_back(name);

	if (usedBoneNames.find(name) != usedBoneNames.end())
	{
		for (const auto& ancestor : ancestors)
		{
			outRequired.insert(ancestor);
		}
	}

	for (uint32_t i = 0; i < node->mNumChildren; ++i)
	{
		CollectRequiredBoneNames(node->mChildren[i], usedBoneNames, outRequired, ancestors);
	}

	ancestors.pop_back();
}

static void CollectRequiredBoneNamesFromList(
	const aiNode* root,
	const std::unordered_set<std::string>& extraBoneNames,
	std::unordered_set<std::string>& outRequired)
{
	if (!root || extraBoneNames.empty())
		return;

	for (const auto& name : extraBoneNames)
	{
		const aiNode* node = FindNodeByName(root, name);
		for (const aiNode* current = node; current; current = current->mParent)
		{
			outRequired.insert(current->mName.C_Str());
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


SkeletonBuildResult BuildSkeletonFromScene(
	const aiScene* scene,
	const std::unordered_set<std::string>& extraBoneNames) 
{
	SkeletonBuildResult out;
	if (!scene || !scene->mRootNode) return out;

	std::unordered_set<std::string> usedBoneNames;
	CollectUsedBoneNames(scene, usedBoneNames);

	// bone이 하나도 없으면 빈 Skeleton 반환(정적 에셋)
	if (usedBoneNames.empty() && extraBoneNames.empty())
		return out;

	std::unordered_set<std::string> requiredBoneNames;
	std::vector<std::string> ancestors;
	ancestors.reserve(64);
	CollectRequiredBoneNames(scene->mRootNode, usedBoneNames, requiredBoneNames, ancestors);
	CollectRequiredBoneNamesFromList(scene->mRootNode, extraBoneNames, requiredBoneNames);

	if (requiredBoneNames.empty())
		return out;

	// 1) aiNode 트리에서 used bone + 부모 노드 등록(순서 고정)
	TraverseAndRegisterBones(scene->mRootNode, requiredBoneNames, out);

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

	std::unordered_set<std::string> extraBoneNames;
	const auto extraBonesJson = skeletonMeta.value("extraBones", nlohmann::json::array());
	if (extraBonesJson.is_array())
	{
		for (const auto& entry : extraBonesJson)
		{
			if (entry.is_string())
			{
				extraBoneNames.insert(entry.get<std::string>());
			}
		}
	}

	const std::string equipmentBoneName = skeletonMeta.value("equipmentBone", std::string("equipment"));
	if (!equipmentBoneName.empty())
	{
		extraBoneNames.insert(equipmentBoneName);
	}

	SkeletonBuildResult skel = BuildSkeletonFromScene(scene, extraBoneNames);

	// bone이 없는 정적 FBX면 skelbin을 안 만들지/빈 파일을 만들지 정책 필요
	// 여기서는 "bone 없으면 false" 대신 "빈 스켈레톤 파일 생성"으로 처리 가능
	// 일단: bone 없으면 outBoneNameToIndex 비우고 true 반환(정적 에셋 케이스)
	outBoneNameToIndex = skel.boneNameToIndex;

#ifdef _DEBUG
	//WriteSkeletonDebug(outSkelBin, skel);
#endif

	if (skel.bones.empty())
		return true;

	SkelBinHeader header{};
	header.version = 4;
	header.boneCount = (uint16_t)skel.bones.size();
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

	const auto boneName = [&skel](int index) -> std::string
		{
			const uint32_t offset = skel.bones[index].nameOffset;
			if (offset >= skel.stringTable.size())
				return std::string();
			return std::string(skel.stringTable.c_str() + offset);
		};

	// 계층 기반 자동 분류.
	// 이름 목록 하드코딩 방식은 목록에 없는 본(spine_04, neck_02, *_metacarpal_*,
	// *_twist_02_*)을 놓쳐서 상체 마스크가 계층 중간에서 끊어진다. 부모와 자식은
	// 마스크에 들어있는데 중간 본만 빠지면 블렌딩 시 그 지점이 꺾여 보인다.
	// 상체 루트의 서브트리를 통째로 잡으면 "연결된 하나의 서브트리"가 구조적으로 보장된다.
	const auto autoFillFromHierarchy = [&]() -> bool
		{
			const int boneCount = static_cast<int>(skel.bones.size());
			if (boneCount == 0)
				return false;

			int upperRoot = -1;
			// 척추 체인의 '최상위' 본이 상체 루트다. Spine1 이 Spine 보다 먼저 오면
			// Spine 하나가 상체에서 빠지므로 순서가 중요하다.
			for (const char* candidate : { "spine_01", "mixamorig:Spine", "Spine", "spine",
										   "Spine1", "Bip01_Spine1" })
			{
				auto it = skel.boneNameToIndex.find(candidate);
				if (it != skel.boneNameToIndex.end())
				{
					upperRoot = static_cast<int>(it->second);
					break;
				}
			}
			if (upperRoot < 0)
				return false; // 상체 루트를 못 찾으면 이름 목록 방식으로 폴백

			std::vector<std::vector<int>> children(boneCount);
			for (int i = 0; i < boneCount; ++i)
			{
				const int parent = skel.bones[i].parentIndex;
				if (parent >= 0 && parent < boneCount)
					children[parent].push_back(i);
			}

			const auto markSubtree = [&children](int start, std::vector<bool>& flags)
				{
					std::vector<int> stack{ start };
					while (!stack.empty())
					{
						const int index = stack.back();
						stack.pop_back();
						if (flags[index])
							continue;
						flags[index] = true;
						for (int child : children[index])
							stack.push_back(child);
					}
				};

			// IK/보조 본은 스키닝 변형에 관여하지 않으므로 서브트리째 분류에서 제외한다.
			std::vector<bool> excluded(boneCount, false);
			for (int i = 0; i < boneCount; ++i)
			{
				const std::string name = boneName(i);
				if (name.rfind("ik_", 0) == 0 || name == "interaction" || name == "center_of_mass")
					markSubtree(i, excluded);
			}

			std::vector<bool> upper(boneCount, false);
			markSubtree(upperRoot, upper);

			for (int i = 0; i < boneCount; ++i)
			{
				if (excluded[i])
					continue;
				if (upper[i])
					upperBodyIndices.push_back(i);
				else
					lowerBodyIndices.push_back(i);
			}

			skeletonMeta["upperBodyBones"] = upperBodyIndices;
			skeletonMeta["lowerBodyBones"] = lowerBodyIndices;
			return true;
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
	else if (!autoFillFromHierarchy())
	{
		autoFillFromUnrealNames();
	}

	header.upperCount = static_cast<uint32_t>(upperBodyIndices.size());
	header.lowerCount = static_cast<uint32_t>(lowerBodyIndices.size());

	const auto setIdentity = [](float out[16])
		{
			for (int i = 0; i < 16; ++i)
			{
				out[i] = (i % 5 == 0) ? 1.0f : 0.0f;
			}
		};

	header.equipmentBoneIndex = -1;
	setIdentity(header.equipmentBindPose);

	if (!equipmentBoneName.empty())
	{
		auto it = skel.boneNameToIndex.find(equipmentBoneName);
		if (it != skel.boneNameToIndex.end())
		{
			const uint32_t index = it->second;
			if (index < skel.bones.size())
			{
				header.equipmentBoneIndex = static_cast<int32_t>(index);
				std::memcpy(header.equipmentBindPose, skel.bones[index].localBind, sizeof(float) * 16);
			}
		}
	}

	std::ofstream ofs(outSkelBin, std::ios::binary);
	if (!ofs) return false;

	const aiNode* rootNode = scene->mRootNode;
	for (const auto& bone : skel.bones)
	{
		if (bone.parentIndex >= 0)
			continue;

		const std::string rootName = ReadStringAtOffset(skel.stringTable, bone.nameOffset);
		if (rootName.empty())
			break;

		if (const aiNode* node = FindNodeByName(scene->mRootNode, rootName))
		{
			rootNode = node;
		}
		break;
	}

	aiMatrix4x4 globalInverse = GetGlobalTransform(rootNode);
	globalInverse.Inverse();
	float globalInverseRowMajor[16]{};
	MatToRowMajor16(globalInverse, globalInverseRowMajor);

	ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));
	ofs.write(reinterpret_cast<const char*>(globalInverseRowMajor), sizeof(float) * 16);

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