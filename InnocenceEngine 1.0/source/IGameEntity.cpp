#include "stdafx.h"
#include "IGameEntity.h"


IGameEntity::IGameEntity()
{
}


IGameEntity::~IGameEntity()
{
}

void IGameEntity::addChildEntity(IGameEntity* childEntity)
{
	m_childGameEntity.emplace_back(childEntity);
	childEntity->setParentEntity(this);
}

std::vector<IGameEntity*>& IGameEntity::getChildrenEntity()
{
	return m_childGameEntity;
}

IGameEntity* IGameEntity::getParentEntity()
{
	return m_parentEntity;
}

void IGameEntity::setParentEntity(IGameEntity* parentEntity)
{
	m_parentEntity = parentEntity;
}

Transform* IGameEntity::getTransform()
{
	return &m_transform;
}


bool IGameEntity::hasTransformChanged()
{
	if (m_transform.getPos() != m_transform.getOldPos() || m_transform.getRot() != m_transform.getOldRot() || m_transform.getScale() != m_transform.getOldScale())
	{
		return true;
	}

	if (getParentEntity() != nullptr)
	{
		if (getParentEntity()->hasTransformChanged())
		{
			return true;
		}
	}
	return false;
}

Mat4f IGameEntity::caclTransformation()
{
	Mat4f l_translationMatrix;
	l_translationMatrix.initTranslation(m_transform.getPos().getX(), m_transform.getPos().getY(), m_transform.getPos().getZ());

	Mat4f l_rotaionMartix = m_transform.getRot().toRotationMatrix();

	Mat4f l_scaleMartix;
	l_scaleMartix.initScale(m_transform.getScale().getX(), m_transform.getScale().getY(), m_transform.getScale().getZ());

	Mat4f l_parentMatrix;
	l_parentMatrix.initIdentity();

	if (getParentEntity() != nullptr && getParentEntity()->hasTransformChanged())
	{
		l_parentMatrix = getParentEntity()->caclTransformation();
	}

	return l_parentMatrix * l_translationMatrix * l_rotaionMartix * l_scaleMartix;
}

Vec3f IGameEntity::caclTransformedPos()
{
	Mat4f l_parentMatrix;
	l_parentMatrix.initIdentity();

	if (getParentEntity() != nullptr && getParentEntity()->hasTransformChanged())
	{
		l_parentMatrix = getParentEntity()->caclTransformation();
	}

	return l_parentMatrix.transform(m_transform.getPos());
}

Vec4f IGameEntity::caclTransformedRot()
{
	Vec4f l_parentRotation = Vec4f(0, 0, 0, 1);

	if (getParentEntity() != nullptr)
	{
		l_parentRotation = getParentEntity()->caclTransformedRot();
	}

	return l_parentRotation * m_transform.getRot();
}


Actor::Actor()
{
}


Actor::~Actor()
{
}

void Actor::init()
{
	for (size_t i = 0; i < getChildrenEntity().size(); i++)
	{
		getChildrenEntity()[i]->exec(INIT);
	}
	this->setStatus(INITIALIZIED);
}

void Actor::update()
{
	if (getChildrenEntity().size() != 0)
	{
		for (size_t i = 0; i < getChildrenEntity().size(); i++)
		{
			getChildrenEntity()[i]->exec(UPDATE);
		}
	}
}

void Actor::shutdown()
{
	for (size_t i = 0; i < getChildrenEntity().size(); i++)
	{
		getChildrenEntity()[i]->exec(SHUTDOWN);
	}
	this->setStatus(UNINITIALIZIED);
}
