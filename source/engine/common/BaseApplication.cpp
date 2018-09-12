#include "BaseApplication.h"
#include "../system/MemorySystem.h"
#include "../system/LogSystem.h"
#include "../system/TaskSystem.h"
#include "../system/TimeSystem.h"
#include "../system/GameSystem.h"
#include "../system/AssetSystem.h"
#include "../system/PhysicsSystem.h"
#include "../system/VisionSystem.h"

void InnoApplication::setup()
{
	InnoMemorySystem::setup();
	InnoLogSystem::setup();
	InnoTaskSystem::setup();
	InnoTimeSystem::setup();
	InnoLogSystem::printLog("MemorySystem setup finished.");
	InnoLogSystem::printLog("LogSystem setup finished.");
	InnoLogSystem::printLog("TaskSystem setup finished.");
	InnoLogSystem::printLog("TimeSystem setup finished.");
	InnoGameSystem::setup();
	InnoLogSystem::printLog("GameSystem setup finished.");
	InnoAssetSystem::setup();
	InnoLogSystem::printLog("AssetSystem setup finished.");
	InnoPhysicsSystem::setup();
	InnoLogSystem::printLog("PhysicsSystem setup finished.");
	InnoVisionSystem::setup();
	InnoLogSystem::printLog("VisionSystem setup finished.");

	m_objectStatus = objectStatus::ALIVE;
	InnoLogSystem::printLog("CoreSystem setup finished.");
}



void InnoApplication::initialize()
{
	InnoMemorySystem::initialize();
	InnoLogSystem::initialize();
	InnoTaskSystem::initialize();
	InnoTimeSystem::initialize();
	InnoGameSystem::initialize();
	InnoAssetSystem::initialize();
	InnoPhysicsSystem::initialize();
	InnoVisionSystem::initialize();
	InnoLogSystem::printLog("CoreSystem has been initialized.");
}

void InnoApplication::update()
{
	// time System should update without any limitation.
	InnoTimeSystem::update();

	InnoGameSystem::update();

	if (InnoVisionSystem::m_VisionSystemStatus == objectStatus::ALIVE)
	{
		if (InnoGameSystem::needRender())
		{
			InnoPhysicsSystem::update();
			InnoVisionSystem::update();
		}
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
		InnoLogSystem::printLog("CoreSystem is stand-by.");
	}
}

void InnoApplication::shutdown()
{
	InnoVisionSystem::shutdown();
	InnoGameSystem::shutdown();
	InnoPhysicsSystem::shutdown();
	InnoAssetSystem::shutdown();
	InnoTimeSystem::shutdown();
	m_objectStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("CoreSystem has been shutdown.");
	InnoLogSystem::shutdown();
	InnoMemorySystem::shutdown();

	std::this_thread::sleep_for(std::chrono::seconds(2));
}
