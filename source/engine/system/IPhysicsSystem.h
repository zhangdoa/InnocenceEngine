#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

INNO_INTERFACE IPhysicsSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IPhysicsSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;
};
