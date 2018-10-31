#pragma once
#include "InnoType.h"
#include "config.h"

namespace InnoApplication
{
#if defined(INNO_RENDERER_DX)
	bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);
#else
	bool setup();
#endif
	bool initialize();
	bool update();
	bool terminate();

	objectStatus getStatus();
};