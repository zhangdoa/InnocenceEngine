#include "../../main/stdafx.h"
#include "IBaseObject.h"


IBaseObject::IBaseObject()
{
	m_gameObjectID = std::rand();
}

IBaseObject::~IBaseObject()
{
}

const objectStatus& IBaseObject::getStatus() const
{
	return m_ObjectStatus;
}

const GameObjectID & IBaseObject::getGameObjectID() const
{
	return m_gameObjectID;
}

void IBaseObject::setStatus(objectStatus ObjectStatus)
{
	m_ObjectStatus = ObjectStatus;
}