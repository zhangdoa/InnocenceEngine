#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "../../common/config.h"

class IWindowSystem
{
public:
	InnoHighLevelSystem_EXPORT virtual bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow) = 0;
	InnoHighLevelSystem_EXPORT virtual bool initialize() = 0;
	InnoHighLevelSystem_EXPORT virtual bool update() = 0;
	InnoHighLevelSystem_EXPORT virtual bool terminate() = 0;

	InnoHighLevelSystem_EXPORT virtual objectStatus getStatus() = 0;

	virtual void swapBuffer() = 0;
};
