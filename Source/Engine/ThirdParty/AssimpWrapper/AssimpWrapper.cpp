#include "AssimpWrapper.h"
#include "AssimpImporter.h"

using namespace Inno;

bool AssimpWrapper::Import(const char* fileName, const char* exportPath)
{
	return AssimpImporter::Import(fileName, exportPath);
}
