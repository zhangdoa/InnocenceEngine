#include "../../main/stdafx.h"
#include "IVisibleGameEntity.h"


IVisibleGameEntity::IVisibleGameEntity()
{
}


IVisibleGameEntity::~IVisibleGameEntity()
{
}

const visibleGameEntityType& IVisibleGameEntity::getVisibleGameEntityType() const
{
	return m_visibleGameEntityType;
}

void IVisibleGameEntity::setVisibleGameEntityType(visibleGameEntityType visibleGameEntityType)
{
	m_visibleGameEntityType = visibleGameEntityType;
}
