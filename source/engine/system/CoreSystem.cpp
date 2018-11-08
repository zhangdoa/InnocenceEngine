#include "CoreSystem.h"
#include "TimeSystem.h"
#include "LogSystem.h"
#include "MemorySystem.h"
#include "TaskSystem.h"
#include "GameSystem.h"
#include "VisionSystem.h"

ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE CoreSystemNS
{
	std::unique_ptr<ITimeSystem> m_TimeSystem;
	std::unique_ptr<ILogSystem> m_LogSystem;
	std::unique_ptr<IMemorySystem> m_MemorySystem;
	std::unique_ptr<ITaskSystem> m_TaskSystem;
	std::unique_ptr<IGameSystem> m_GameSystem;
	std::unique_ptr<IVisionSystem> m_VisionSystem;
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
	CoreSystemNS::m_TaskSystem = std::make_unique<InnoTaskSystem>();
	if (!CoreSystemNS::m_TaskSystem.get())
	{
		return false;
	}
	CoreSystemNS::m_GameSystem = std::make_unique<InnoGameSystem>();
	if (!CoreSystemNS::m_TaskSystem.get())
	{
		return false;
	}
	CoreSystemNS::m_VisionSystem = std::make_unique<InnoVisionSystem>();
	if (!CoreSystemNS::m_TaskSystem.get())
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

INNO_SYSTEM_EXPORT ITaskSystem * InnoCoreSystem::getTaskSystem()
{
	return CoreSystemNS::m_TaskSystem.get();
}

INNO_SYSTEM_EXPORT IGameSystem * InnoCoreSystem::getGameSystem()
{
	return 	CoreSystemNS::m_GameSystem.get();
}

INNO_SYSTEM_EXPORT IVisionSystem * InnoCoreSystem::getVisionSystem()
{
	return CoreSystemNS::m_VisionSystem.get();
}
