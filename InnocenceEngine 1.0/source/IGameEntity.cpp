#include "stdafx.h"
#include "IGameEntity.h"


BaseActor::BaseActor()
{
}


BaseActor::~BaseActor()
{
}

void BaseActor::addChildActor(BaseActor* childActor)
{
	m_childActor.emplace_back(childActor);
	childActor->setParentActor(this);
}

std::vector<BaseActor*>& BaseActor::getChildrenActors()
{
	return m_childActor;
}

BaseActor* BaseActor::getParentActor()
{
	return m_parentActor;
}

void BaseActor::setParentActor(BaseActor* parentActor)
{
	m_parentActor = parentActor;
}

void BaseActor::addChildComponent(BaseComponent * childComponent)
{
	m_childComponent.emplace_back(childComponent);
	childComponent->setParentActor(this);
}

std::vector<BaseComponent*>& BaseActor::getChildrenComponents()
{
	return m_childComponent;
}

Transform* BaseActor::getTransform()
{
	return &m_transform;
}


bool BaseActor::hasTransformChanged()
{
	if (m_transform.getPos() != m_transform.getOldPos() || m_transform.getRot() != m_transform.getOldRot() || m_transform.getScale() != m_transform.getOldScale())
	{
		return true;
	}

	if (getParentActor() != nullptr)
	{
		if (getParentActor()->hasTransformChanged())
		{
			return true;
		}
	}
	return false;
}

Mat4f BaseActor::caclTransformation()
{
	Mat4f l_translationMatrix;
	l_translationMatrix.initTranslation(m_transform.getPos().getX(), m_transform.getPos().getY(), m_transform.getPos().getZ());

	Mat4f l_rotaionMartix = m_transform.getRot().toRotationMatrix();

	Mat4f l_scaleMartix;
	l_scaleMartix.initScale(m_transform.getScale().getX(), m_transform.getScale().getY(), m_transform.getScale().getZ());

	Mat4f l_parentMatrix;
	l_parentMatrix.initIdentity();

	if (getParentActor() != nullptr && getParentActor()->hasTransformChanged())
	{
		l_parentMatrix = getParentActor()->caclTransformation();
	}

	return l_parentMatrix * l_translationMatrix * l_rotaionMartix * l_scaleMartix;
}

Vec3f BaseActor::caclTransformedPos()
{
	Mat4f l_parentMatrix;
	l_parentMatrix.initIdentity();

	if (getParentActor() != nullptr && getParentActor()->hasTransformChanged())
	{
		l_parentMatrix = getParentActor()->caclTransformation();
	}

	return l_parentMatrix.transform(m_transform.getPos());
}

Vec4f BaseActor::caclTransformedRot()
{
	Vec4f l_parentRotation = Vec4f(0, 0, 0, 1);

	if (getParentActor() != nullptr)
	{
		l_parentRotation = getParentActor()->caclTransformedRot();
	}

	return l_parentRotation * m_transform.getRot();
}


void BaseActor::init()
{
	for (size_t i = 0; i < m_childActor.size(); i++)
	{
		m_childActor[i]->exec(INIT);
	}
	for (size_t i = 0; i < m_childComponent.size(); i++)
	{
		m_childComponent[i]->exec(INIT);
	}
	this->setStatus(INITIALIZIED);
}

void BaseActor::update()
{
	if (m_childActor.size() != 0)
	{
		for (size_t i = 0; i < getChildrenActors().size(); i++)
		{
			m_childActor[i]->exec(UPDATE);
		}
	}
	if (m_childComponent.size() != 0)
	{
		for (size_t i = 0; i < m_childComponent.size(); i++)
		{
			m_childComponent[i]->exec(UPDATE);
		}
	}
}

void BaseActor::shutdown()
{
	for (size_t i = 0; i < getChildrenActors().size(); i++)
	{
		getChildrenActors()[i]->exec(SHUTDOWN);
	}
	for (size_t i = 0; i < m_childComponent.size(); i++)
	{
		m_childComponent[i]->exec(SHUTDOWN);
	}
	this->setStatus(UNINITIALIZIED);
}

BaseComponent::BaseComponent()
{
}

BaseComponent::~BaseComponent()
{
}

BaseActor * BaseComponent::getParentActor()
{
	return m_parentActor;
}

void BaseComponent::setParentActor(BaseActor * parentActor)
{
	m_parentActor = parentActor;
}

Transform * BaseComponent::getTransform()
{
	return m_parentActor->getTransform();
}
