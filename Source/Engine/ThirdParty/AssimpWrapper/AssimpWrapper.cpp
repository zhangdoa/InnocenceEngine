#include "AssimpWrapper.h"

namespace Inno
{
	namespace AssimpWrapper
	{
		bool Import(const char* fileName)
		{
			return AssimpImporter::Import(fileName);
		}
	}
}
