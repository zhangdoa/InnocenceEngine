#pragma once
#include "../../common/InnoType.h"

INNO_PRIVATE_SCOPE InnoFileSystemNS
{
	INNO_PRIVATE_SCOPE AssimpWrapper
	{
		bool convertModel(const std::string & fileName, const std::string & exportPath);
	};
}