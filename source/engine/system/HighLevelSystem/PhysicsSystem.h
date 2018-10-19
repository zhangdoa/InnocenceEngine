#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/ComponentHeaders.h"

namespace InnoPhysicsSystem
{
	InnoHighLevelSystem_EXPORT bool setup();
	InnoHighLevelSystem_EXPORT bool initialize();
	InnoHighLevelSystem_EXPORT bool update();
	InnoHighLevelSystem_EXPORT bool terminate();

	InnoHighLevelSystem_EXPORT objectStatus getStatus();
};
