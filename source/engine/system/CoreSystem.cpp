#include "CoreSystem.h"
#include "Core/TimeSystem.h"
#include "Core/LogSystem.h"
#include "Core/MemorySystem.h"
#include "Core/TaskSystem.h"
#include "TestSystem.h"
#include "FileSystem.h"
#include "GameSystem.h"
#include "AssetSystem.h"
#include "PhysicsSystem.h"
#include "InputSystem.h"
#include "VisionSystem.h"

ICoreSystem* g_pCoreSystem;

#define createSubSystemInstanceDefi( className ) \
m_##className = std::make_unique<Inno##className>(); \
if (!m_##className.get()) \
{ \
	return false; \
} \

#define subSystemSetup( className ) \
if (!g_pCoreSystem->get##className()->setup()) \
{ \
	return false; \
} \
g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, std::string(#className) + " setup finished."); \

#define subSystemInit( className ) \
if (!g_pCoreSystem->get##className()->initialize()) \
{ \
	return false; \
} \

//#define INNO_TEST_BUILD
#if defined INNO_TEST_BUILD
#define subSystemUpdate( className ) \
{ \
std::function<void()> l_task = [](){ g_pCoreSystem->get##className()->update(); }; \
g_pCoreSystem->getTestSystem()->measure(std::string(#className), l_task); \
}
#else
#define subSystemUpdate( className ) \
if (!g_pCoreSystem->get##className()->update()) \
{ \
return false; \
}
#endif

#define subSystemTerm( className ) \
if (!g_pCoreSystem->get##className()->terminate()) \
{ \
	return false; \
} \

#define subSystemGetDefi( className ) \
INNO_SYSTEM_EXPORT I##className * InnoCoreSystem::get##className() \
{ \
	return InnoCoreSystemNS::m_##className.get(); \
} \

INNO_PRIVATE_SCOPE InnoCoreSystemNS
{
	bool createSubSystemInstance();
	bool setup(void* appHook, void* extraHook, char* pScmdline);
	bool initialize();
	bool update();
	bool terminate();

	std::unique_ptr<ITimeSystem> m_TimeSystem;
	std::unique_ptr<ILogSystem> m_LogSystem;
	std::unique_ptr<IMemorySystem> m_MemorySystem;
	std::unique_ptr<ITaskSystem> m_TaskSystem;
	std::unique_ptr<ITestSystem> m_TestSystem;
	std::unique_ptr<IFileSystem> m_FileSystem;
	std::unique_ptr<IGameSystem> m_GameSystem;
	std::unique_ptr<IAssetSystem> m_AssetSystem;
	std::unique_ptr<IPhysicsSystem> m_PhysicsSystem;
	std::unique_ptr<IInputSystem> m_InputSystem;
	std::unique_ptr<IVisionSystem> m_VisionSystem;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

bool InnoCoreSystemNS::createSubSystemInstance()
{
	createSubSystemInstanceDefi(TimeSystem);
	createSubSystemInstanceDefi(LogSystem);
	createSubSystemInstanceDefi(MemorySystem);
	createSubSystemInstanceDefi(TaskSystem);

	createSubSystemInstanceDefi(TestSystem);
	createSubSystemInstanceDefi(FileSystem);
	createSubSystemInstanceDefi(GameSystem);
	createSubSystemInstanceDefi(AssetSystem);
	createSubSystemInstanceDefi(PhysicsSystem);
	createSubSystemInstanceDefi(InputSystem);
	createSubSystemInstanceDefi(VisionSystem);

	return true;
}

bool InnoCoreSystemNS::setup(void* appHook, void* extraHook, char* pScmdline)
{
	if (!InnoCoreSystemNS::createSubSystemInstance())
	{
		return false;
	}

	subSystemSetup(TimeSystem);
	subSystemSetup(LogSystem);
	subSystemSetup(MemorySystem);
	subSystemSetup(TaskSystem);

	subSystemSetup(TestSystem);

	if (!g_pCoreSystem->getVisionSystem()->setup(appHook, extraHook, pScmdline))
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VisionSystem setup finished.");

	subSystemSetup(AssetSystem);
	subSystemSetup(FileSystem);
	subSystemSetup(GameSystem);
	subSystemSetup(PhysicsSystem);
	subSystemSetup(InputSystem);

	m_objectStatus = ObjectStatus::STANDBY;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine setup finished.");
	return true;
}

bool InnoCoreSystemNS::initialize()
{
	subSystemInit(TimeSystem);
	subSystemInit(LogSystem);
	subSystemInit(MemorySystem);
	subSystemInit(TaskSystem);

	subSystemInit(TestSystem);

	subSystemInit(FileSystem);
	subSystemInit(GameSystem);
	subSystemInit(AssetSystem);
	subSystemInit(PhysicsSystem);
	subSystemInit(InputSystem);
	subSystemInit(VisionSystem);

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been initialized.");
	return true;
}

bool InnoCoreSystemNS::update()
{
	subSystemUpdate(TimeSystem);
	subSystemUpdate(LogSystem);
	subSystemUpdate(MemorySystem);
	subSystemUpdate(TaskSystem);

	subSystemUpdate(TestSystem);

	subSystemUpdate(FileSystem);
	subSystemUpdate(GameSystem);
	subSystemUpdate(AssetSystem);
	subSystemUpdate(PhysicsSystem);
	subSystemUpdate(InputSystem);

#if defined INNO_TEST_BUILD
	{
		std::function<void()> l_task = [&]() {
			if (g_pCoreSystem->getVisionSystem()->getStatus() == ObjectStatus::ALIVE)
			{
				if (!g_pCoreSystem->getVisionSystem()->update())
				{
					m_objectStatus = ObjectStatus::STANDBY;
				}
				g_pCoreSystem->getGameSystem()->saveComponentsCapture();
			}
			else
			{
				m_objectStatus = ObjectStatus::STANDBY;
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "Engine is stand-by.");
			}
		};
		g_pCoreSystem->getTestSystem()->measure("VisionSystem", l_task);
	}
#else
	if (g_pCoreSystem->getVisionSystem()->getStatus() == ObjectStatus::ALIVE)
	{
		if (!g_pCoreSystem->getVisionSystem()->update())
		{
			return false;
		}
		g_pCoreSystem->getGameSystem()->saveComponentsCapture();
	}
	else
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "Engine is stand-by.");
		return false;
	}
#endif

	return true;
}

bool InnoCoreSystemNS::terminate()
{
	subSystemTerm(VisionSystem);
	subSystemTerm(InputSystem);
	subSystemTerm(PhysicsSystem);
	subSystemTerm(AssetSystem);
	subSystemTerm(GameSystem);
	subSystemTerm(FileSystem);

	subSystemTerm(TestSystem);

	subSystemTerm(TaskSystem);
	subSystemTerm(MemorySystem);
	subSystemTerm(LogSystem);
	subSystemTerm(TimeSystem);

	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been terminated.");

	return true;
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::setup(void* appHook, void* extraHook, char* pScmdline)
{
	g_pCoreSystem = this;

	return InnoCoreSystemNS::setup(appHook, extraHook, pScmdline);
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::initialize()
{
	return InnoCoreSystemNS::initialize();
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::update()
{
	return InnoCoreSystemNS::update();
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::terminate()
{
	return InnoCoreSystemNS::terminate();
}

INNO_SYSTEM_EXPORT ObjectStatus InnoCoreSystem::getStatus()
{
	return InnoCoreSystemNS::m_objectStatus;
}

subSystemGetDefi(TimeSystem);
subSystemGetDefi(LogSystem);
subSystemGetDefi(MemorySystem);
subSystemGetDefi(TaskSystem);

subSystemGetDefi(TestSystem);

subSystemGetDefi(FileSystem);
subSystemGetDefi(GameSystem);
subSystemGetDefi(AssetSystem);
subSystemGetDefi(PhysicsSystem);
subSystemGetDefi(InputSystem);
subSystemGetDefi(VisionSystem);