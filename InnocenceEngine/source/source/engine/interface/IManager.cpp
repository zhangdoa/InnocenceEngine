#include "../../main/stdafx.h"
#include "IManager.h"

const objectStatus& IManager::getStatus() const
{
	return m_ObjectStatus;
}

void IManager::setStatus(objectStatus ObjectStatus)
{
	m_ObjectStatus = ObjectStatus;
}