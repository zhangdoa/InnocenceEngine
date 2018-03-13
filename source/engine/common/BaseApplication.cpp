#include "BaseApplication.h"

void BaseApplication::setup()
{
	g_pCoreManager->setup();
	m_objectStatus = objectStatus::ALIVE;
}

void BaseApplication::initialize()
{
	if (g_pCoreManager->getStatus() == objectStatus::ALIVE)
	{
		g_pCoreManager->initialize();
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
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
		m_objectStatus = objectStatus::STANDBY;
	}
}

void BaseApplication::shutdown()
{
	g_pCoreManager->shutdown();
	m_objectStatus = objectStatus::SHUTDOWN;
}

const objectStatus & BaseApplication::getStatus() const
{
	return m_objectStatus;
}