#pragma once
#include "../../common/InnoType.h"

namespace InnoVisionSystem
{
	__declspec(dllexport) void setup();
	__declspec(dllexport) void initialize();
	__declspec(dllexport) void update();
	__declspec(dllexport) void shutdown();

	__declspec(dllexport) objectStatus getStatus();
};
