#include "stdafx.h"
#include "IEventManager.h"



IEventManager::IEventManager()
{
}


IEventManager::~IEventManager()
{
}

void IEventManager::exec(execMessage execMessage)
{
	switch (execMessage)
	{
	case INIT: init(); m_managerStatus = INITIALIZIED;  break;
	case UPDATE: update(); break;
	case SHUTDOWN: shutdown(); m_managerStatus = UNINITIALIZIED; break;
	default: printLog("Unknown error!"); m_managerStatus = ERROR;
		break;
	}
}

const int IEventManager::getStatus() const
{
	return m_managerStatus;
}

void IEventManager::setStatus(managerStatus managerStatus)
{
	m_managerStatus = managerStatus;
}

void IEventManager::printLog(std::string logMessage)
{
	std::cout << logMessage << std::endl;
}
