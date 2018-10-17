#pragma once
#include "../../common/ComponentHeaders.h"

namespace InnoPhysicsSystem
{
	__declspec(dllexport) void setup();
	__declspec(dllexport) void initialize();
	__declspec(dllexport) void update();
	__declspec(dllexport) void shutdown();

	__declspec(dllexport) objectStatus getStatus();
};
