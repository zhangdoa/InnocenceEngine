#include "../../main/stdafx.h"
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
	case  execMessage::INIT: init(); m_ObjectStatus = objectStatus::INITIALIZIED;  break;
	case  execMessage::UPDATE: update(); break;
	case  execMessage::SHUTDOWN: shutdown(); m_ObjectStatus = objectStatus::UNINITIALIZIED; break;
	default: m_ObjectStatus = objectStatus::ERROR; break;
	}
}

const objectStatus& IBaseObject::getStatus() const
{
	return m_ObjectStatus;
}

void IBaseObject::setStatus(objectStatus ObjectStatus)
{
	m_ObjectStatus = ObjectStatus;
}