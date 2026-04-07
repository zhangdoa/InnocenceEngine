#pragma once
#include "../../Common/ComponentHeaders.h"
#include "nlohmann/json.hpp"


struct aiScene;
struct aiNode;

namespace Inno
{
	namespace AssimpImporter
	{
		bool Import(const char* FileName);
		void ProcessAssimpScene(nlohmann::json& J, const aiScene* Scene, const char* ExportName);
		void ProcessAssimpNode(const aiNode* Node, const aiScene* Scene, const char* BaseName,
			std::vector<std::pair<std::string, std::string>>& drawCalls);
	}
}
