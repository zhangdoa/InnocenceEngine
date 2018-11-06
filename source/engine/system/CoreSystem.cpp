#include "CoreSystem.h"
#include "TimeSystem.h"
#include "LogSystem.h"
#include "MemorySystem.h"

ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE CoreSystemNS
{
	std::unique_ptr<ITimeSystem> m_TimeSystem;
	std::unique_ptr<ILogSystem> m_LogSystem;
	std::unique_ptr<IMemorySystem> m_MemorySystem;
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::setup()
{
	g_pCoreSystem = this;

	CoreSystemNS::m_TimeSystem = std::make_unique<InnoTimeSystem>();
	if (!CoreSystemNS::m_TimeSystem.get())
	{
		return false;
	}
	CoreSystemNS::m_LogSystem = std::make_unique<InnoLogSystem>();
	if (!CoreSystemNS::m_LogSystem.get())
	{
		return false;
	}
	CoreSystemNS::m_MemorySystem = std::make_unique<InnoMemorySystem>();
	if (!CoreSystemNS::m_MemorySystem.get())
	{
		return false;
	}
	return true;
}

INNO_SYSTEM_EXPORT ITimeSystem * InnoCoreSystem::getTimeSystem()
{
	return CoreSystemNS::m_TimeSystem.get();
}

INNO_SYSTEM_EXPORT ILogSystem * InnoCoreSystem::getLogSystem()
{
	return CoreSystemNS::m_LogSystem.get();
}

INNO_SYSTEM_EXPORT IMemorySystem * InnoCoreSystem::getMemorySystem()
{
	return CoreSystemNS::m_MemorySystem.get();
}
