#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "../../common/config.h"

namespace InnoVisionSystem
{
#if defined(INNO_RENDERER_DX)
	InnoHighLevelSystem_EXPORT bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);
#else
	InnoHighLevelSystem_EXPORT bool setup();
#endif
	InnoHighLevelSystem_EXPORT bool initialize();
	InnoHighLevelSystem_EXPORT bool update();
	InnoHighLevelSystem_EXPORT bool terminate();

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};
