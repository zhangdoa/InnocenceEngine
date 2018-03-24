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

mat4 BaseEntity::caclLocalPosMatrix()
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

vec3 BaseEntity::caclWorldPos()
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
	return vec3(l_parentTransformationMatrix.m[0][0] * m_transform.getPos().x + l_parentTransformationMatrix.m[0][1] * m_transform.getPos().y + l_parentTransformationMatrix.m[0][2] * m_transform.getPos().z,
		l_parentTransformationMatrix.m[1][0] * m_transform.getPos().x + l_parentTransformationMatrix.m[1][1] * m_transform.getPos().y + l_parentTransformationMatrix.m[1][2] * m_transform.getPos().z,
		l_parentTransformationMatrix.m[2][0] * m_transform.getPos().x + l_parentTransformationMatrix.m[2][1] * m_transform.getPos().y + l_parentTransformationMatrix.m[2][2] * m_transform.getPos().z);
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	return vec3(l_parentTransformationMatrix.m[0][0] * m_transform.getPos().x + l_parentTransformationMatrix.m[0][1] * m_transform.getPos().y + l_parentTransformationMatrix.m[0][2] * m_transform.getPos().z,
		l_parentTransformationMatrix.m[1][0] * m_transform.getPos().x + l_parentTransformationMatrix.m[1][1] * m_transform.getPos().y + l_parentTransformationMatrix.m[1][2] * m_transform.getPos().z,
		l_parentTransformationMatrix.m[2][0] * m_transform.getPos().x + l_parentTransformationMatrix.m[2][1] * m_transform.getPos().y + l_parentTransformationMatrix.m[2][2] * m_transform.getPos().z);
#endif
}

quat BaseEntity::caclWorldRot()
{
	quat l_parentRotationQuat = quat(0, 0, 0, 1);

	if (m_parentEntity != nullptr)
	{
		l_parentRotationQuat = m_parentEntity->caclWorldRot();
	}

	return l_parentRotationQuat.mul(m_transform.getRot());
}

vec3 BaseEntity::caclWorldScale()
{
	vec3 l_parentScale = vec3(1.0, 1.0, 1.0);

	if (m_parentEntity != nullptr)
	{
		l_parentScale = m_parentEntity->caclWorldScale();
	}

	return l_parentScale.mul(m_transform.getScale());
}

mat4 BaseEntity::caclWorldPosMatrix()
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
	return l_parentTransformationMatrix * caclLocalPosMatrix() * caclLocalRotMatrix() * caclLocalScaleMatrix();
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	return caclLocalScaleMatrix() *  caclLocalRotMatrix() *  caclLocalPosMatrix()  * l_parentTransformationMatrix;
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
