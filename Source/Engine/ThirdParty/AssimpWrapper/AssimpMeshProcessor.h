#pragma once
#include "../../Common/ComponentHeaders.h"
#include "nlohmann/json.hpp"
#include <vector>

struct aiMesh;
struct aiScene;

namespace Inno
{
	namespace AssimpMeshProcessor
	{
		// Create and save MeshComponent directly - returns pointer for linking
		MeshComponent* CreateMeshComponent(const aiScene* scene, const char* exportName, uint32_t meshIndex);
		
		// Convert Assimp mesh data to our format
		size_t ConvertMeshData(const aiMesh* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices);
		
		// Process bones if present
		void ProcessAssimpBone(nlohmann::json& j, const aiMesh* mesh);
	}
}
