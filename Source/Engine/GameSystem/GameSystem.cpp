#include "GameSystem.h"
#include "../Common/Config.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE InnoGameSystemNS
{
	bool setup();
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	EntityChildrenComponentsMetadataMap m_entityChildrenComponentsMetadataMap;

	unsigned int m_currentUUID = 0;

	std::function<void()> f_sceneLoadingStartCallback;
}

bool InnoGameSystemNS::setup()
{
	return true;
}

bool InnoGameSystem::setup()
{
	if (!InnoGameSystemNS::setup())
	{
		return false;
	}

	InnoGameSystemNS::f_sceneLoadingStartCallback = [&]() {
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&InnoGameSystemNS::f_sceneLoadingStartCallback);

	InnoGameSystemNS::m_objectStatus = ObjectStatus::Created;

	return true;
}

bool InnoGameSystem::initialize()
{
	if (InnoGameSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoGameSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been initialized.");
		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "GameSystem: Object is not created!");
		return false;
	}
}

bool InnoGameSystem::update()
{
	if (InnoGameSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		InnoGameSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool InnoGameSystem::terminate()
{
	InnoGameSystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem has been terminated.");
	return true;
}

std::string InnoGameSystem::getGameName()
{
	return std::string("GameInstance");
}

const EntityChildrenComponentsMetadataMap& InnoGameSystem::getEntityChildrenComponentsMetadataMap()
{
	return InnoGameSystemNS::m_entityChildrenComponentsMetadataMap;
}

ObjectStatus InnoGameSystem::getStatus()
{
	return InnoGameSystemNS::m_objectStatus;
}