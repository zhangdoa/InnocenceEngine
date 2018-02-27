#include "BaseApplication.h"

void BaseApplication::setup()
{
	m_pCoreManager = new CoreManager();
	m_pCoreManager->setup();
}

void BaseApplication::initialize()
{
	m_pCoreManager->initialize();
	setStatus(objectStatus::ALIVE);
}

void BaseApplication::update()
{
	if (m_pCoreManager->getStatus() == objectStatus::ALIVE)
	{
		m_pCoreManager->update();
	}
	else
	{
		setStatus(objectStatus::STANDBY);
	}
}

void BaseApplication::shutdown()
{
	m_pCoreManager->shutdown();
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
