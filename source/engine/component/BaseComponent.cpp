#include "BaseComponent.h"

EntityID BaseComponent::getParentEntity() const
{
	return m_parentEntity;
}

void BaseComponent::setParentEntity(EntityID parentEntity)
{
	m_parentEntity = parentEntity;
}

const objectStatus & BaseComponent::getStatus() const
{
	return m_objectStatus;
}