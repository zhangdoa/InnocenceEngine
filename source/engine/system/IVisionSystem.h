#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

INNO_INTERFACE IVisionSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IVisionSystem);

	INNO_SYSTEM_EXPORT virtual bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow) = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;


	INNO_SYSTEM_EXPORT virtual objectStatus getStatus() = 0;
};
