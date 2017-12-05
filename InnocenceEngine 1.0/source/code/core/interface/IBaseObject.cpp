#include "../../main/stdafx.h"
#include "IBaseObject.h"


IBaseObject::IBaseObject()
{
}

IBaseObject::~IBaseObject()
{
}

const objectStatus& IBaseObject::getStatus() const
{
	return m_ObjectStatus;
}

void IBaseObject::setStatus(objectStatus ObjectStatus)
{
	m_ObjectStatus = ObjectStatus;
}