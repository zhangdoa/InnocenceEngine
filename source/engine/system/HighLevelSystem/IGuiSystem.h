#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "../../common/config.h"

class IGuiSystem
{
public:
	InnoHighLevelSystem_EXPORT virtual bool setup() = 0;
	InnoHighLevelSystem_EXPORT virtual bool initialize() = 0;
	InnoHighLevelSystem_EXPORT virtual bool update() = 0;
	InnoHighLevelSystem_EXPORT virtual bool terminate() = 0;

	InnoHighLevelSystem_EXPORT virtual objectStatus getStatus() = 0;
};
