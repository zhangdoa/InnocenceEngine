#pragma once
#include "../../Common/ComponentHeaders.h"
#include "nlohmann/json.hpp"
#include "../../Common/STL14.h"

struct aiMesh;
struct aiScene;

namespace Inno
{
	namespace AssimpMeshProcessor
	{
		bool CreateMeshComponent(const aiScene* Scene, const char* BaseName, uint32_t MeshIndex, MeshComponent& OutMesh);

		size_t ConvertMeshData(const aiMesh* Mesh, std::vector<Vertex>& Vertices, std::vector<Index>& Indices);

		void ProcessAssimpBone(nlohmann::json& J, const aiMesh* Mesh);
	}
}
