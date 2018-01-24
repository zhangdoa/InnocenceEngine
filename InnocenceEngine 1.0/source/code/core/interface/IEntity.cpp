#include "../../main/stdafx.h"
#include "IEntity.h"

IEntity::IEntity()
{
	m_entityID = std::rand();
	m_className = std::string{ typeid(*this).name() };
	m_className = m_className.substr(m_className.find("class"), std::string::npos);
}

IEntity::~IEntity()
{
}

const EntityID & IEntity::getEntityID() const
{
	return m_entityID;
}

const std::string & IEntity::getClassName() const
{
	return m_className;
}
