#pragma once
#include "ICoreSystem.h"

class InnoCoreSystem : INNO_IMPLEMENT ICoreSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoCoreSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT ITimeSystem* getTimeSystem() override;
	INNO_SYSTEM_EXPORT ILogSystem* getLogSystem() override;
	INNO_SYSTEM_EXPORT IMemorySystem* getMemorySystem() override;
};