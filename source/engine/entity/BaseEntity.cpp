#include "BaseEntity.h"

BaseEntity::BaseEntity()
{
	m_entityID = std::rand();
}

BaseEntity::~BaseEntity()
{
}

void BaseEntity::addChildEntity(IEntity* childEntity)
{
	m_childEntitys.emplace_back(childEntity);
	childEntity->setParentEntity(this);
}

const std::vector<IEntity*>& BaseEntity::getChildrenEntitys() const
{
	return m_childEntitys;
}

IEntity* BaseEntity::getParentEntity() const
{
	return m_parentEntity;
}

void BaseEntity::setParentEntity(IEntity* parentActor)
{
	m_parentEntity = parentActor;
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

Transform* BaseEntity::getTransform()
{
	return &m_transform;
}

bool BaseEntity::hasTransformChanged()
{
	if (m_transform.getPos() != m_transform.getOldPos() || m_transform.getRot() != m_transform.getOldRot() || m_transform.getScale() != m_transform.getOldScale())
	{
		return true;
	}

	if (m_parentEntity != nullptr)
	{
		if (m_parentEntity->hasTransformChanged())
		{
			return true;
		}
	}
	return false;
}

mat4 BaseEntity::caclLocalTranslationMatrix()
{
	return m_transform.getPos().toTranslationMartix();
}

mat4 BaseEntity::caclLocalRotMatrix()
{
	return m_transform.getRot().toRotationMartix();
}

mat4 BaseEntity::caclLocalScaleMatrix()
{
	return m_transform.getScale().toScaleMartix();
}

vec4 BaseEntity::caclWorldPos()
{
	mat4 l_parentTransformationMatrix;
	l_parentTransformationMatrix.m[0][0] = 1.0f;
	l_parentTransformationMatrix.m[1][1] = 1.0f;
	l_parentTransformationMatrix.m[2][2] = 1.0f;
	l_parentTransformationMatrix.m[3][3] = 1.0f;

	if (m_parentEntity != nullptr && m_parentEntity->hasTransformChanged())
	{
		l_parentTransformationMatrix = m_parentEntity->caclTransformationMatrix();
	}

	return l_parentTransformationMatrix.mul(m_transform.getPos());
}

vec4 BaseEntity::caclWorldRot()
{
	vec4 l_parentRot = vec4(0, 0, 0, 1);

	if (m_parentEntity != nullptr)
	{
		l_parentRot = m_parentEntity->caclWorldRot();
	}

	return l_parentRot.quatMul(m_transform.getRot());
}

vec4 BaseEntity::caclWorldScale()
{
	vec4 l_parentScale = vec4(1.0, 1.0, 1.0, 1.0);

	if (m_parentEntity != nullptr)
	{
		l_parentScale = m_parentEntity->caclWorldScale();
	}

	return l_parentScale.scale(m_transform.getScale());
}

mat4 BaseEntity::caclWorldTranslationMatrix()
{
	return caclWorldPos().toTranslationMartix();
}

mat4 BaseEntity::caclWorldRotMatrix()
{
	return caclWorldRot().toRotationMartix();
}

mat4 BaseEntity::caclWorldScaleMatrix()
{
	return caclWorldScale().toScaleMartix();
}

mat4 BaseEntity::caclTransformationMatrix()
{
	mat4 l_parentTransformationMatrix;

	l_parentTransformationMatrix.m[0][0] = 1.0f;
	l_parentTransformationMatrix.m[1][1] = 1.0f;
	l_parentTransformationMatrix.m[2][2] = 1.0f;
	l_parentTransformationMatrix.m[3][3] = 1.0f;

	if (m_parentEntity != nullptr && m_parentEntity->hasTransformChanged())
	{
		l_parentTransformationMatrix = m_parentEntity->caclTransformationMatrix();
	}

	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	return l_parentTransformationMatrix * caclLocalTranslationMatrix() * caclLocalRotMatrix() * caclLocalScaleMatrix();
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	return caclLocalScaleMatrix() * caclLocalRotMatrix() * caclLocalTranslationMatrix() * l_parentTransformationMatrix;
#endif
}

void BaseEntity::setup()
{
	for (auto l_childComponent : m_childComponents)
	{
		l_childComponent->setup();
	}
	for (auto l_childActor : m_childEntitys)
	{
		l_childActor->setup();
	}
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
	m_objectStatus = objectStatus::ALIVE;
}

void BaseEntity::update()
{
	m_transform.update();
	for (auto l_childActor : m_childEntitys)
	{
		l_childActor->update();
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
