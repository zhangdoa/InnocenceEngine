#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

#include "IRenderingFrontendSystem.h"

INNO_INTERFACE IRenderingBackendSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingBackendSystem);

	INNO_SYSTEM_EXPORT virtual bool setup(IRenderingFrontendSystem* renderingFrontend) = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual bool resize() = 0;
};
