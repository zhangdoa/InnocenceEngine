#include "BaseApplication.h"

void BaseApplication::setup()
{
	CoreManager::getInstance().setup();
}

void BaseApplication::initialize()
{
	CoreManager::getInstance().initialize();
	setStatus(objectStatus::ALIVE);
}

void BaseApplication::update()
{
	if (CoreManager::getInstance().getStatus() == objectStatus::ALIVE)
	{
		CoreManager::getInstance().update();
	}
	else
	{
		setStatus(objectStatus::STANDBY);
	}
}

void BaseApplication::shutdown()
{
	CoreManager::getInstance().shutdown();
	setStatus(objectStatus::SHUTDOWN);
}

const objectStatus & BaseApplication::getStatus() const
{
	return m_objectStatus;
}

void BaseApplication::setStatus(objectStatus objectStatus)
{
	m_objectStatus = objectStatus;
}
