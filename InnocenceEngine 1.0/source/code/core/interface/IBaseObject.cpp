#include "../../main/stdafx.h"
#include "IBaseObject.h"


IBaseObject::IBaseObject()
{
	m_gameObjectID = std::rand();
	m_className = std::string{ typeid(*this).name() };
	m_className = m_className.substr(m_className.find("class"), std::string::npos);
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

const std::string & IBaseObject::getClassName() const
{
	return m_className;
}

void IBaseObject::setStatus(objectStatus ObjectStatus)
{
	m_ObjectStatus = ObjectStatus;
}