#include "BaseApplication.h"

void BaseApplication::setup()
{
	g_pCoreSystem->setup();
	m_objectStatus = objectStatus::ALIVE;
}

void BaseApplication::initialize()
{
	if (g_pCoreSystem->getStatus() == objectStatus::ALIVE)
	{
		g_pCoreSystem->initialize();
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
	}
}

void BaseApplication::update()
{
	if (g_pCoreSystem->getStatus() == objectStatus::ALIVE)
	{
		g_pCoreSystem->update();
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
	}
}

void BaseApplication::shutdown()
{
	g_pCoreSystem->shutdown();
	m_objectStatus = objectStatus::SHUTDOWN;
}

const objectStatus & BaseApplication::getStatus() const
{
	return m_objectStatus;
}
