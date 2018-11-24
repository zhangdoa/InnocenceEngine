#pragma once
#include "IPhysicsSystem.h"

class InnoPhysicsSystem : INNO_IMPLEMENT IPhysicsSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoPhysicsSystem);

	INNO_SYSTEM_EXPORT bool setup();
	INNO_SYSTEM_EXPORT bool initialize();
	INNO_SYSTEM_EXPORT bool update();
	INNO_SYSTEM_EXPORT bool terminate();

	INNO_SYSTEM_EXPORT ObjectStatus getStatus();
};
