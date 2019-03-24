#pragma once
#include "InnoType.h"

namespace InnoApplication
{
	bool setup(void* hInstance, char* pScmdline);
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus getStatus();
};