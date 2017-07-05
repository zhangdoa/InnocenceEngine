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
}

IGameEntity * IGameEntity::getParentEntity()
{
	return m_parentEntity;
}

void IGameEntity::setParentEntity(IGameEntity * parentEntity)
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
	Mat4f translationMatrix;
	translationMatrix.initTranslation(m_transform.getPos().getX(), m_transform.getPos().getY(), m_transform.getPos().getZ());

	Quaternion _tempRot = m_transform.getRot();
	Mat4f rotaionMartix = _tempRot.toRotationMatrix();

	Mat4f scaleMartix;
	scaleMartix.initScale(m_transform.getScale().getX(), m_transform.getScale().getY(), m_transform.getScale().getZ());

	Mat4f m_parentMatrix;
	m_parentMatrix.initIdentity();
	if (getParentEntity() != nullptr && getParentEntity()->hasTransformChanged())
	{
		m_parentMatrix = getParentEntity()->caclTransformation();
	}

	return m_parentMatrix * translationMatrix * rotaionMartix * scaleMartix;
}

Vec3f IGameEntity::caclTransformedPos()
{
	Mat4f m_parentMatrix;
	m_parentMatrix.initIdentity();
	if (getParentEntity() != nullptr && getParentEntity()->hasTransformChanged())
	{
		m_parentMatrix = getParentEntity()->caclTransformation();
	}

	return m_parentMatrix.transform(m_transform.getPos());
}

Quaternion IGameEntity::caclTransformedRot()
{
	Quaternion m_parentRotation = Quaternion(0, 0, 0, 1);

	if (getParentEntity() != nullptr)

		m_parentRotation = getParentEntity()->caclTransformedRot();

	return m_parentRotation * m_transform.getRot();
}


Actor::Actor()
{
}


Actor::~Actor()
{
}
