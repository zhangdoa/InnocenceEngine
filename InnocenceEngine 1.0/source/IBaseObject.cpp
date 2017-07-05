#include "stdafx.h"
#include "IBaseObject.h"


IBaseObject::IBaseObject()
{
}


IBaseObject::~IBaseObject()
{
}

void IBaseObject::exec(execMessage execMessage)
{
	switch (execMessage)
	{
	case INIT: init(); m_ObjectStatus = INITIALIZIED;  break;
	case UPDATE: update(); break;
	case SHUTDOWN: shutdown(); m_ObjectStatus = UNINITIALIZIED; break;
	default: printLog("Unknown error!"); m_ObjectStatus = ERROR;
		break;
	}
}

const int IBaseObject::getStatus() const
{
	return m_ObjectStatus;
}

void IBaseObject::setStatus(managerStatus ObjectStatus)
{
	m_ObjectStatus = ObjectStatus;
}

void IBaseObject::printLog(std::string logMessage)
{
	std::cout << logMessage << std::endl;
}