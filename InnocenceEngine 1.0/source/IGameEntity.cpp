#include "stdafx.h"
#include "IGameEntity.h"

Transform::Transform()
{
	m_pos = glm::vec3(0.0f, 0.0f, 0.0f);
	m_rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
	m_oldPos = m_pos + (1.0f);
	m_oldRot = m_rot *(0.5f);
	m_oldScale = m_scale + (1.0f);
}

Transform::~Transform()
{
}

void Transform::update()
{
	m_oldPos = m_pos;
	m_oldRot = m_rot;
	m_oldScale = m_scale;
}

void Transform::rotate(const glm::vec3& axis, float angle)
{
	float sinHalfAngle = sinf(angle / 2);
	float cosHalfAngle = cosf(angle / 2);

	glm::quat rotateFactor;

	rotateFactor.w = cosHalfAngle;
	rotateFactor.x = axis.x * sinHalfAngle;
	rotateFactor.y = axis.y * sinHalfAngle;
	rotateFactor.z = axis.z * sinHalfAngle;

	m_rot = (glm::normalize(rotateFactor * m_rot));
}

const glm::vec3 & Transform::getPos() const
{
	return m_pos;
}

const glm::quat & Transform::getRot() const
{
	return m_rot;
}

const glm::vec3 & Transform::getScale() const
{
	return m_scale;
}

void Transform::setPos(const glm::vec3 & pos)
{
	m_pos = pos;
}

void Transform::setRot(const glm::quat & rot)
{
	m_rot = rot;
}

void Transform::setScale(const glm::vec3 & scale)
{
	m_scale = scale;
}

const glm::vec3 & Transform::getOldPos() const
{
	return m_oldPos;
}

const glm::quat & Transform::getOldRot() const
{
	return m_oldRot;
}

const glm::vec3 & Transform::getOldScale() const
{
	return m_oldScale;
}

glm::vec3 Transform::getDirection(direction direction) const
{
	glm::vec3 l_directionVec3;

	switch (direction)
	{
	case FORWARD: l_directionVec3 = glm::vec3(0.0f, 0.0f, 1.0f); break;
	case BACKWARD:l_directionVec3 = glm::vec3(0.0f, 0.0f, -1.0f); break;
	case UP:l_directionVec3 = glm::vec3(0.0f, 1.0f, 0.0f); break;
	case DOWN:l_directionVec3 = glm::vec3(0.0f, -1.0f, 0.0f); break;
	case RIGHT:l_directionVec3 = glm::vec3(1.0f, 0.0f, 0.0f); break;
	case LEFT:l_directionVec3 = glm::vec3(-1.0f, 0.0f, 0.0f); break;
	}

	// get conjugate quaternion
	glm::quat l_conjugateQuat;

	l_conjugateQuat.w = m_rot.w;
	l_conjugateQuat.x = -m_rot.x;
	l_conjugateQuat.y = -m_rot.y;
	l_conjugateQuat.z = -m_rot.z;

	// get rotated quaternion
	glm::quat l_rotatedRot;

	l_rotatedRot.w = -m_rot.x * l_directionVec3.x - m_rot.y * l_directionVec3.y - m_rot.z * l_directionVec3.z;
	l_rotatedRot.x = m_rot.w * l_directionVec3.x + m_rot.y * l_directionVec3.z - m_rot.z * l_directionVec3.y;
	l_rotatedRot.y = m_rot.w * l_directionVec3.y + m_rot.z * l_directionVec3.x - m_rot.x * l_directionVec3.z;
	l_rotatedRot.z = m_rot.w * l_directionVec3.z + m_rot.x * l_directionVec3.y - m_rot.y * l_directionVec3.x;

	// multiply rotated quaternion by the conjugate quaternion
	l_rotatedRot.w = l_rotatedRot.w * l_conjugateQuat.w - l_rotatedRot.x * l_conjugateQuat.x - l_rotatedRot.y * l_conjugateQuat.y - l_rotatedRot.z * l_conjugateQuat.z;
	l_rotatedRot.x = l_rotatedRot.x * l_conjugateQuat.w + l_rotatedRot.w * l_conjugateQuat.x + l_rotatedRot.y * l_conjugateQuat.z - l_rotatedRot.z * l_conjugateQuat.y;
	l_rotatedRot.y = l_rotatedRot.y * l_conjugateQuat.w + l_rotatedRot.w * l_conjugateQuat.y + l_rotatedRot.z * l_conjugateQuat.x - l_rotatedRot.x * l_conjugateQuat.z;
	l_rotatedRot.z = l_rotatedRot.z * l_conjugateQuat.w + l_rotatedRot.w * l_conjugateQuat.z + l_rotatedRot.x * l_conjugateQuat.y - l_rotatedRot.y * l_conjugateQuat.x;

	// TODO: fix mathmetic error
	return glm::normalize(glm::vec3(l_rotatedRot.x, l_rotatedRot.y, l_rotatedRot.z));
}
glm::mat4 Transform::QuatToRotationMatrix(const glm::quat & quat) const
{
	glm::vec3 forward = glm::vec3(2.0f * (quat.x * quat.z - quat.w * quat.y), 2.0f * (quat.y * quat.z + quat.w * quat.x), 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y));
	glm::vec3 up = glm::vec3(2.0f * (quat.x * quat.y + quat.w * quat.z), 1.0f - 2.0f * (quat.x * quat.x + quat.z * quat.z), 2.0f * (quat.y * quat.z - quat.w * quat.x));
	glm::vec3 right = glm::vec3(1.0f - 2.0f * (quat.y * quat.y + quat.z * quat.z), 2.0f * (quat.x * quat.y - quat.w * quat.z), 2.0f * (quat.x * quat.z + quat.w * quat.y));
	glm::mat4 rotationMatrix;
	rotationMatrix[0][0] = right.x;
	rotationMatrix[0][1] = right.y;
	rotationMatrix[0][2] = right.z;
	rotationMatrix[0][3] = 0;
	rotationMatrix[1][0] = up.x;
	rotationMatrix[1][1] = up.y;
	rotationMatrix[1][2] = up.z;
	rotationMatrix[1][3] = 0;
	rotationMatrix[2][0] = forward.x;
	rotationMatrix[2][1] = forward.y;
	rotationMatrix[2][2] = forward.z;
	rotationMatrix[2][3] = 0;
	rotationMatrix[3][0] = 0;
	rotationMatrix[3][1] = 0;
	rotationMatrix[3][2] = 0;
	rotationMatrix[3][3] = 1;
	
	return rotationMatrix;
}

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

const std::vector<BaseActor*>& BaseActor::getChildrenActors() const
{
	return m_childActor;
}

const BaseActor& BaseActor::getParentActor() const
{
	return *m_parentActor;
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

const std::vector<BaseComponent*>& BaseActor::getChildrenComponents() const
{
	return m_childComponent;
}

Transform* BaseActor::getTransform()
{
	return &m_transform;
}


bool BaseActor::hasTransformChanged() const
{
	if (m_transform.getPos() != m_transform.getOldPos() || m_transform.getRot() != m_transform.getOldRot() || m_transform.getScale() != m_transform.getOldScale())
	{
		return true;
	}

	if (&getParentActor() != nullptr)
	{
		if (getParentActor().hasTransformChanged())
		{
			return true;
		}
	}
	return false;
}

glm::mat4 BaseActor::caclTransformation() const
{
	glm::mat4 l_translationMatrix;
	l_translationMatrix = glm::translate(l_translationMatrix, m_transform.getPos());

	glm::mat4 l_rotaionMartix = m_transform.QuatToRotationMatrix(m_transform.getRot());

	glm::mat4 l_scaleMatrix;
	l_scaleMatrix = glm::scale(l_scaleMatrix, m_transform.getScale());

	glm::mat4 l_parentMatrix;
	
	l_parentMatrix[0][0] = 1.0f;
	l_parentMatrix[0][1] = 0.0f;
	l_parentMatrix[0][2] = 0.0f;
	l_parentMatrix[0][3] = 0.0f;
	l_parentMatrix[1][0] = 0.0f;
	l_parentMatrix[1][1] = 1.0f;
	l_parentMatrix[1][2] = 0.0f;
	l_parentMatrix[1][3] = 0.0f;
	l_parentMatrix[2][0] = 0.0f;
	l_parentMatrix[2][1] = 0.0f;
	l_parentMatrix[2][2] = 1.0f;
	l_parentMatrix[2][3] = 0.0f;
	l_parentMatrix[3][0] = 0.0f;
	l_parentMatrix[3][1] = 0.0f;
	l_parentMatrix[3][2] = 0.0f;
	l_parentMatrix[3][3] = 1.0f;

	if (&getParentActor() != nullptr && getParentActor().hasTransformChanged())
	{
		l_parentMatrix = getParentActor().caclTransformation();
	}

	return l_parentMatrix * l_translationMatrix * l_rotaionMartix * l_scaleMatrix;
}

glm::vec3 BaseActor::caclTransformedPos() const
{
	glm::mat4 l_parentMatrix;

	l_parentMatrix[0][0] = 1.0f;
	l_parentMatrix[0][1] = 0.0f;
	l_parentMatrix[0][2] = 0.0f;
	l_parentMatrix[0][3] = 0.0f;
	l_parentMatrix[1][0] = 0.0f;
	l_parentMatrix[1][1] = 1.0f;
	l_parentMatrix[1][2] = 0.0f;
	l_parentMatrix[1][3] = 0.0f;
	l_parentMatrix[2][0] = 0.0f;
	l_parentMatrix[2][1] = 0.0f;
	l_parentMatrix[2][2] = 1.0f;
	l_parentMatrix[2][3] = 0.0f;
	l_parentMatrix[3][0] = 0.0f;
	l_parentMatrix[3][1] = 0.0f;
	l_parentMatrix[3][2] = 0.0f;
	l_parentMatrix[3][3] = 1.0f;

	if (&getParentActor() != nullptr && getParentActor().hasTransformChanged())
	{
		l_parentMatrix = getParentActor().caclTransformation();
	}

	return glm::vec3(l_parentMatrix[0][0] * m_transform.getPos().x + l_parentMatrix[0][1] * m_transform.getPos().y + l_parentMatrix[0][2] * m_transform.getPos().z + l_parentMatrix[0][3],
		l_parentMatrix[1][0] * m_transform.getPos().x + l_parentMatrix[1][1] * m_transform.getPos().y + l_parentMatrix[1][2] * m_transform.getPos().z + l_parentMatrix[1][3],
		l_parentMatrix[2][0] * m_transform.getPos().x + l_parentMatrix[2][1] * m_transform.getPos().y + l_parentMatrix[2][2] * m_transform.getPos().z + l_parentMatrix[2][3]);
}

glm::quat BaseActor::caclTransformedRot() const
{
	glm::quat l_parentRotation = glm::quat(1, 0, 0, 0);

	if (&getParentActor() != nullptr)
	{
		l_parentRotation = getParentActor().caclTransformedRot();
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

const BaseActor& BaseComponent::getParentActor() const
{
	return *m_parentActor;
}

void BaseComponent::setParentActor(BaseActor * parentActor)
{
	m_parentActor = parentActor;
}

Transform* BaseComponent::getTransform()
{
	return m_parentActor->getTransform();
}
