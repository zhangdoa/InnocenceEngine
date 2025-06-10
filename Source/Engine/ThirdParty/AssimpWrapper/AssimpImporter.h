#pragma once
#include "../../Common/ComponentHeaders.h"
#include "nlohmann/json.hpp"
#include <functional>

struct aiScene;
struct aiNode;

namespace Inno
{
	namespace AssimpImporter
	{
		bool Import(const char* fileName);
		void ProcessAssimpScene(nlohmann::json& j, const aiScene* scene, const char* exportName);
		void ProcessAssimpNode(const aiNode* node, const aiScene* scene, const char* exportName, ModelComponent* modelComponent);
	}
}
