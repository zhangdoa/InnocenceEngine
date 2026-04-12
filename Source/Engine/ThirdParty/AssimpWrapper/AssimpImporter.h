#pragma once
#include "../../Common/ComponentHeaders.h"

struct aiScene;
struct aiNode;

namespace Inno
{
	namespace AssimpImporter
	{
		bool Import(const char* FileName);
		void ProcessAssimpScene(const aiScene* Scene, const char* ExportName, const char* ModelBaseDir);
		void ProcessAssimpNode(const aiNode* Node, const aiScene* Scene, const char* BaseName, const char* ModelBaseDir,
			std::vector<std::pair<std::string, std::string>>& drawCalls);
	}
}
