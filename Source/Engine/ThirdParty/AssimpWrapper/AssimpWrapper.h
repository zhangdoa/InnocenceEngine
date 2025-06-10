#pragma once
#include "AssimpImporter.h"

namespace Inno
{
	namespace AssimpWrapper
	{
		// Main import function - delegates to AssimpImporter
		bool Import(const char* fileName);
	}
}

