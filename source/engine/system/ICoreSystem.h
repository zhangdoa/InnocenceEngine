#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"
#include "../exports/InnoSystem_Export.h"
#include "ITimeSystem.h"

INNO_INTERFACE ICoreSystem
{
public: 
	INNO_POLYMORPHISM_TYPE_CONCRETE(ICoreSystem)
	INNO_ASSIGNMENT_TYPE_NON_COPYABLE(ICoreSystem)
	INNO_ASSIGNMENT_TYPE_MOVABLE(ICoreSystem)

	INNO_SYSTEM_EXPORT virtual ITimeSystem* getTimeSystem() = 0;
};