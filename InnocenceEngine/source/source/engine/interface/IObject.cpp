#include "../../main/stdafx.h"
#include "IObject.h"

IObject::IObject()
{
}

IObject::~IObject()
{
}

const objectStatus& IObject::getStatus() const
{
	return m_ObjectStatus;
}

void IObject::setStatus(objectStatus ObjectStatus)
{
	m_ObjectStatus = ObjectStatus;
}