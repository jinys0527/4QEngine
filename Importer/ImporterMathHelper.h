#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

inline aiVector3D Cross3(const aiVector3D& V1, const aiVector3D& V2)
{
	return aiVector3D(
		V1.y * V2.z - V1.z * V2.y,
		V1.z * V2.x - V1.x * V2.z,
		V1.x * V2.y - V1.y * V2.x
	);
}

inline float Dot3(const aiVector3D& V1, const aiVector3D& V2)
{
	return V1.x * V2.x + V1.y * V2.y + V1.z * V2.z;
}