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

void InnoApplication::setup()
{
	InnoTimeSystem::setup();
	InnoLogSystem::setup();
	InnoMemorySystem::setup();
	InnoTaskSystem::setup();
	InnoLogSystem::printLog("TimeSystem setup finished.");
	InnoLogSystem::printLog("LogSystem setup finished.");
	InnoLogSystem::printLog("MemorySystem setup finished.");
	InnoLogSystem::printLog("TaskSystem setup finished.");

	InnoGameSystem::setup();
	InnoLogSystem::printLog("GameSystem setup finished.");
	InnoAssetSystem::setup();
	InnoLogSystem::printLog("AssetSystem setup finished.");
	InnoPhysicsSystem::setup();
	InnoLogSystem::printLog("PhysicsSystem setup finished.");
	InnoVisionSystem::setup();
	InnoLogSystem::printLog("VisionSystem setup finished.");

	m_objectStatus = objectStatus::ALIVE;

	InnoLogSystem::printLog("Engine setup finished.");
}



void InnoApplication::initialize()
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
}

void InnoApplication::update()
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
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;

		InnoLogSystem::printLog("Engine is stand-by.");
	}
}

void InnoApplication::shutdown()
{
	InnoVisionSystem::shutdown();
	InnoPhysicsSystem::shutdown();

	InnoAssetSystem::shutdown();
	InnoGameSystem::shutdown();

	InnoMemorySystem::shutdown();
	InnoTaskSystem::shutdown();
	InnoLogSystem::shutdown();
	InnoTimeSystem::shutdown();

	m_objectStatus = objectStatus::SHUTDOWN;
}

objectStatus InnoApplication::getStatus()
{
	return m_objectStatus;
}

