#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"

namespace InnoVisionSystem
{
	InnoHighLevelSystem_EXPORT bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);
	InnoHighLevelSystem_EXPORT bool initialize();
	InnoHighLevelSystem_EXPORT bool update();
	InnoHighLevelSystem_EXPORT bool terminate();

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};
