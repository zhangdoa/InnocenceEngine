#include "AssimpWrapper.h"
#include "AssimpImporter.h"

using namespace Inno;

bool AssimpWrapper::Import(const char* fileName)
{
	return AssimpImporter::Import(fileName);
}
