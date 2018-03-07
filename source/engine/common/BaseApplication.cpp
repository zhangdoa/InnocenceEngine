#include "BaseApplication.h"

void BaseApplication::setup()
{
	g_pCoreManager->setup();
	setStatus(objectStatus::ALIVE);
}

void BaseApplication::initialize()
{
	if (g_pCoreManager->getStatus() == objectStatus::ALIVE)
	{
		g_pCoreManager->initialize();
	}
	else
	{
		setStatus(objectStatus::STANDBY);
	}
}

void BaseApplication::update()
{
	if (g_pCoreManager->getStatus() == objectStatus::ALIVE)
	{
		g_pCoreManager->update();
	}
	else
	{
		setStatus(objectStatus::STANDBY);
	}
}

void BaseApplication::shutdown()
{
	g_pCoreManager->shutdown();
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
