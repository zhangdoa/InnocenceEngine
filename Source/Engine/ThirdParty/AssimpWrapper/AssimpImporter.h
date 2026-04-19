#pragma once
#include "../../Common/ComponentHeaders.h"

struct aiScene;

namespace Inno
{
	namespace AssimpImporter
	{
		bool Import(const char* FileName);
		void ProcessAssimpScene(const aiScene* Scene, const char* ExportName, const char* ModelBaseDir);
	}
}
