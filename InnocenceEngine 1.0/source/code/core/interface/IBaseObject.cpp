#include "../../main/stdafx.h"
#include "IBaseObject.h"


IBaseObject::IBaseObject()
{
}


IBaseObject::~IBaseObject()
{
}

void IBaseObject::excute(executeMessage execteMessage)
{
	switch (execteMessage)
	{
	case  executeMessage::INITIALIZE: initialize(); m_ObjectStatus = objectStatus::ALIVE;  break;
	case  executeMessage::UPDATE: update(); break;
	case  executeMessage::SHUTDOWN: shutdown(); m_ObjectStatus = objectStatus::SHUTDOWN; break;
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