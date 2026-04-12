#pragma once
#include "AssimpImporter.h"
#include "../../Common/AssetImportData.h"

namespace Inno
{
	namespace AssimpWrapper
	{
		// Main import function - delegates to AssimpImporter
		bool Import(const char* fileName);
	}
}
