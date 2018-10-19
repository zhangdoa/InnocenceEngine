#include "InnoApplication.h"
#include "../exports/LowLevelSystem_Export.h"
#include "../exports/HighLevelSystem_Export.h"
#include "../system/LowLevelSystem/TimeSystem.h"
#include "../system/LowLevelSystem/LogSystem.h"
#include "../system/LowLevelSystem/MemorySystem.h"
#include "../system/LowLevelSystem/TaskSystem.h"
#include "../system/HighLevelSystem/GameSystem.h"
#include "../system/HighLevelSystem/AssetSystem.h"
#include "../system/HighLevelSystem/PhysicsSystem.h"
#include "../system/HighLevelSystem/VisionSystem.h"

namespace InnoApplication
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}
#if defined(INNO_RENDERER_DX)
#include "../component/DXWindowSystemSingletonComponent.h"
bool InnoApplication::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{	
#else
bool InnoApplication::setup()
{
#endif
	if (!InnoTimeSystem::setup())
	{
		return false;
	}
	InnoLogSystem::printLog("TimeSystem setup finished.");

	if (!InnoLogSystem::setup())
	{
		return false;
	}
	InnoLogSystem::printLog("LogSystem setup finished.");

	if (!InnoMemorySystem::setup())
	{
		return false;
	}
	InnoLogSystem::printLog("MemorySystem setup finished.");

	if (!InnoTaskSystem::setup())
	{
		return false;
	}
	InnoLogSystem::printLog("TaskSystem setup finished.");

	if (!InnoGameSystem::setup())
	{
		return false;
	}
	InnoLogSystem::printLog("GameSystem setup finished.");

	if (!InnoAssetSystem::setup())
	{
		return false;
	}
	InnoLogSystem::printLog("AssetSystem setup finished.");

	if (!InnoPhysicsSystem::setup())
	{
		return false;
	}
	InnoLogSystem::printLog("PhysicsSystem setup finished.");

#if defined(INNO_RENDERER_DX)
	if (!InnoVisionSystem::setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
	{
		return false;
	}
#else
	if (!InnoVisionSystem::setup();)
	{
		return false;
	}
#endif
	InnoLogSystem::printLog("VisionSystem setup finished.");

	m_objectStatus = objectStatus::ALIVE;

	InnoLogSystem::printLog("Engine setup finished.");
	return true;
}



bool InnoApplication::initialize()
{
	InnoTimeSystem::initialize();
	InnoLogSystem::initialize();
	InnoMemorySystem::initialize();
	InnoTaskSystem::initialize();
	InnoGameSystem::initialize();
	InnoAssetSystem::initialize();
	InnoPhysicsSystem::initialize();
	InnoVisionSystem::initialize();

	InnoLogSystem::printLog("Engine has been initialized.");
	return true;
}

bool InnoApplication::update()
{
	InnoTimeSystem::update();
	InnoLogSystem::update();
	InnoMemorySystem::update();
	InnoTaskSystem::update();

	InnoGameSystem::update();
	InnoAssetSystem::update();
	InnoPhysicsSystem::update();

	if (InnoVisionSystem::getStatus() == objectStatus::ALIVE)
	{	
		InnoVisionSystem::update();
		return true;
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
		InnoLogSystem::printLog("Engine is stand-by.");
		return false;
	}
}

bool InnoApplication::terminate()
{
	InnoVisionSystem::terminate();
	InnoPhysicsSystem::terminate();

	InnoAssetSystem::terminate();
	InnoGameSystem::terminate();

	InnoMemorySystem::terminate();
	InnoTaskSystem::terminate();
	InnoLogSystem::terminate();
	InnoTimeSystem::terminate();

	m_objectStatus = objectStatus::SHUTDOWN;
	return true;
}

objectStatus InnoApplication::getStatus()
{
	return m_objectStatus;
}

