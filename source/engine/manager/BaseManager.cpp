#include "BaseManager.h"

const objectStatus & BaseManager::getStatus() const
{
	return m_objectStatus;
}

void BaseManager::setStatus(objectStatus objectStatus)
{
	m_objectStatus = objectStatus;
}