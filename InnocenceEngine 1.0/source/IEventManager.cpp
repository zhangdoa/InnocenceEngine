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
	case INIT: init(); m_managerStatus = RUNNING;  break;
	case UPDATE: update(); m_managerStatus = RUNNING; break;
	case SHUTDOWN: shutdown(); m_managerStatus = UNINITIALIZIED; break;
	default: printLog("Unknown error!"); m_managerStatus = ERROR;
		break;
	}
}

int IEventManager::getStatus()
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
