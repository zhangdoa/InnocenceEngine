#include "InnoApplication.h"
#include "../exports/LowLevelSystem_Export.h"
#include "../exports/HighLevelSystem_Export.h"
#include "../system/LowLevelSystem/TimeSystem.h"
#include "../system/LowLevelSystem/LogSystem.h"
#include "../system/LowLevelSystem/MemorySystem.h"
#include "../system/LowLevelSystem/TaskSystem.h"
//#include "../system/HighLevelSystem/GameSystem.h"
//#include "../system/HighLevelSystem/AssetSystem.h"
//#include "../system/HighLevelSystem/PhysicsSystem.h"
//#include "../system/HighLevelSystem/VisionSystem.h"
//
#include "../component/LogSystemSingletonComponent.h"
//#include "../component/GameSystemSingletonComponent.h"

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
	LogSystemSingletonComponent::getInstance().m_log.push("TimeSystem setup finished.");
	LogSystemSingletonComponent::getInstance().m_log.push("LogSystem setup finished.");
	LogSystemSingletonComponent::getInstance().m_log.push("MemorySystem setup finished.");
	LogSystemSingletonComponent::getInstance().m_log.push("TaskSystem setup finished.");

	//InnoGameSystem::setup();
	//LogSystemSingletonComponent::getInstance().m_log.push("GameSystem setup finished.");
	//InnoAssetSystem::setup();
	//LogSystemSingletonComponent::getInstance().m_log.push("AssetSystem setup finished.");
	//InnoPhysicsSystem::setup();
	//LogSystemSingletonComponent::getInstance().m_log.push("PhysicsSystem setup finished.");
	//InnoVisionSystem::setup();
	//LogSystemSingletonComponent::getInstance().m_log.push("VisionSystem setup finished.");

	m_objectStatus = objectStatus::ALIVE;

	LogSystemSingletonComponent::getInstance().m_log.push("Engine setup finished.");
}



void InnoApplication::initialize()
{
	InnoTimeSystem::initialize();
	InnoLogSystem::initialize();
	InnoMemorySystem::initialize();
	InnoTaskSystem::initialize();
	//InnoGameSystem::initialize();
	//InnoAssetSystem::initialize();
	//InnoPhysicsSystem::initialize();
	//InnoVisionSystem::initialize();

	LogSystemSingletonComponent::getInstance().m_log.push("Engine has been initialized.");
}

void InnoApplication::update()
{
	InnoTimeSystem::update();
	InnoLogSystem::update();
	InnoMemorySystem::update();
	InnoTaskSystem::update();

	//InnoGameSystem::update();
	//InnoAssetSystem::update();

	//if (InnoVisionSystem::getStatus() == objectStatus::ALIVE)
	//{
	//	if (GameSystemSingletonComponent::getInstance().m_needRender)
	//	{
	//		InnoPhysicsSystem::update();
	//		InnoVisionSystem::update();
	//	}
	//}
	//else
	//{
	//	m_objectStatus = objectStatus::STANDBY;

	//	LogSystemSingletonComponent::getInstance().m_log.push("Engine is stand-by.");
	//}
}

void InnoApplication::shutdown()
{
	//InnoVisionSystem::shutdown();
	//InnoPhysicsSystem::shutdown();

	//InnoAssetSystem::shutdown();
	//InnoGameSystem::shutdown();

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

