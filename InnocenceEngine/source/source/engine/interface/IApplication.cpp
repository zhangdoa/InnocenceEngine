#include "../../main/stdafx.h"
#include "IApplication.h"

const objectStatus & IApplication::getStatus() const
{
	return m_objectStatus;
}

void IApplication::setStatus(objectStatus objectStatus)
{
	m_objectStatus = objectStatus;
}
