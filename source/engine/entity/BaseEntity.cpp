#include "BaseEntity.h"

BaseEntity::BaseEntity()
{
	m_entityID = std::rand();
}

BaseEntity::~BaseEntity()
{
}

void BaseEntity::addChildComponent(IComponent * childComponent)
{
	m_childComponents.emplace_back(childComponent);
	childComponent->setParentEntity(this);
}

const std::vector<IComponent*>& BaseEntity::getChildrenComponents() const
{
	return m_childComponents;
}

void BaseEntity::setup()
{
	for (auto l_childComponent : m_childComponents)
	{
		l_childComponent->setup();
	}
	m_objectStatus = objectStatus::ALIVE;
}

void BaseEntity::initialize()
{
	for (auto l_childComponent : m_childComponents)
	{
		l_childComponent->initialize();
	}
	for (auto l_childActor : m_childEntitys)
	{
		l_childActor->initialize();
	}
}

void BaseEntity::shutdown()
{
	for (auto& l_childActor : m_childEntitys)
	{
		l_childActor->shutdown();
	}
	for (auto l_childComponent : m_childComponents)
	{
		l_childComponent->shutdown();
	}
	m_objectStatus = objectStatus::SHUTDOWN;
}

const EntityID & BaseEntity::getEntityID() const
{
	return m_entityID;
}

const objectStatus & BaseEntity::getStatus() const
{
	return m_objectStatus;
}
