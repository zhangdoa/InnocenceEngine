#pragma once
#include "ICoreSystem.h"

class CoreSystem : INNO_IMPLEMENT ICoreSystem
{
public:
	INNO_SYSTEM_EXPORT ITimeSystem* getTimeSystem() override;
};
