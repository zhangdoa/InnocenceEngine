#pragma once
#include "InnoType.h"

namespace InnoApplication
{
	bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);
	bool initialize();
	bool update();
	bool terminate();

	objectStatus getStatus();
};