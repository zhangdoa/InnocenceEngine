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
	default: m_ObjectStatus = ERROR; break;
	}
}

const int IBaseObject::getStatus() const
{
	return m_ObjectStatus;
}

void IBaseObject::setStatus(objectStatus ObjectStatus)
{
	m_ObjectStatus = ObjectStatus;
}