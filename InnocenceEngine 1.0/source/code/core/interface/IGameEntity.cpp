#include "../../main/stdafx.h"
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
	float sinHalfAngle = glm::sin((angle * glm::pi<float>() / 180.0f) / 2.0f);
	float cosHalfAngle = glm::cos((angle * glm::pi<float>() / 180.0f) / 2.0f);
	// get final rotation
	m_rot = glm::normalize(glm::quat(cosHalfAngle, axis.x * sinHalfAngle, axis.y * sinHalfAngle, axis.z * sinHalfAngle) * m_rot);
}

glm::vec3 & Transform::getPos()
{
	return m_pos;
}

glm::quat & Transform::getRot()
{
	return m_rot;
}

glm::vec3 & Transform::getScale()
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

glm::vec3 & Transform::getOldPos()
{
	return m_oldPos;
}
glm::quat & Transform::getOldRot()
{
	return m_oldRot;
}

glm::vec3 & Transform::getOldScale()
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

	// V' = QVQ^-1, for unit quaternion, conjugated quaternion is same as inverse quatertion

	// naive version

	//// get Q * V by hand
	////glm::quat l_hiddenRotatedQuat;
	////l_hiddenRotatedQuat.w = -m_rot.x * l_directionVec3.x - m_rot.y * l_directionVec3.y - m_rot.z * l_directionVec3.z;
	////l_hiddenRotatedQuat.x = m_rot.w * l_directionVec3.x + m_rot.y * l_directionVec3.z - m_rot.z * l_directionVec3.y;
	////l_hiddenRotatedQuat.y = m_rot.w * l_directionVec3.y + m_rot.z * l_directionVec3.x - m_rot.x * l_directionVec3.z;
	////l_hiddenRotatedQuat.z = m_rot.w * l_directionVec3.z + m_rot.x * l_directionVec3.y - m_rot.y * l_directionVec3.x;

	//// get conjugate quaternion
	////glm::quat l_conjugateQuat;
	////l_conjugateQuat = glm::conjugate(m_rot);

	//// then QV * Q^-1 
	////glm::quat l_directionQuat;
	////l_directionQuat = l_hiddenRotatedQuat * l_conjugateQuat;
	////l_directionVec3.x = l_directionQuat.x;
	////l_directionVec3.y = l_directionQuat.y;
	////l_directionVec3.z = l_directionQuat.z;

	// traditional version, change direction vector to quaternion representation

	////glm::quat l_directionQuat = glm::quat(0.0, l_directionVec3);
	////l_directionQuat = m_rot * l_directionQuat * glm::conjugate(m_rot);
	////l_directionVec3.x = l_directionQuat.x;
	////l_directionVec3.y = l_directionQuat.y;
	////l_directionVec3.z = l_directionQuat.z;

	// optimized version ([Kavan et al. ] Lemma 4)
	//V' = V + 2 * Qv x (Qv x V +Qs * V)
	glm::vec3 l_Qv = glm::vec3(m_rot.x, m_rot.y, m_rot.z);
	l_directionVec3 = l_directionVec3 + glm::cross(2.0f * l_Qv, (glm::cross(l_Qv, l_directionVec3) + m_rot.w * l_directionVec3));
	
	return l_directionVec3;
}

BaseActor::BaseActor()
{
}


BaseActor::~BaseActor()
{
}

void BaseActor::addChildActor(BaseActor* childActor)
{
	m_childActors.emplace_back(childActor);
	childActor->setParentActor(this);
}

const std::vector<BaseActor*>& BaseActor::getChildrenActors() const
{
	return m_childActors;
}

BaseActor* BaseActor::getParentActor() const
{
	return m_parentActor;
}

void BaseActor::setParentActor(BaseActor* parentActor)
{
	m_parentActor = parentActor;
}

void BaseActor::addChildComponent(BaseComponent * childComponent)
{
	m_childComponents.emplace_back(childComponent);
	childComponent->setParentActor(this);
}

const std::vector<BaseComponent*>& BaseActor::getChildrenComponents() const
{
	return m_childComponents;
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

glm::mat4 BaseActor::caclLocalPosMatrix()
{
	return glm::translate(glm::mat4(), m_transform.getPos());
}

glm::mat4 BaseActor::caclLocalRotMatrix()
{
	return glm::toMat4(m_transform.getRot());
}

glm::mat4 BaseActor::caclLocalScaleMatrix()
{
	return glm::scale(glm::mat4(), m_transform.getScale());
}

glm::vec3 BaseActor::caclWorldPos()
{
	glm::mat4 l_parentTransformationMatrix;
	l_parentTransformationMatrix[0][0] = 1.0f;
	l_parentTransformationMatrix[1][1] = 1.0f;
	l_parentTransformationMatrix[2][2] = 1.0f;
	l_parentTransformationMatrix[3][3] = 1.0f;

	if (getParentActor() != nullptr && getParentActor()->hasTransformChanged())
	{
		l_parentTransformationMatrix = getParentActor()->caclTransformationMatrix();
	}

	return glm::vec3(l_parentTransformationMatrix[0][0] * m_transform.getPos().x + l_parentTransformationMatrix[1][0] * m_transform.getPos().y + l_parentTransformationMatrix[2][0] * m_transform.getPos().z + l_parentTransformationMatrix[3][0],
		l_parentTransformationMatrix[0][1] * m_transform.getPos().x + l_parentTransformationMatrix[1][1] * m_transform.getPos().y + l_parentTransformationMatrix[2][1] * m_transform.getPos().z + l_parentTransformationMatrix[3][1],
		l_parentTransformationMatrix[0][2] * m_transform.getPos().x + l_parentTransformationMatrix[1][2] * m_transform.getPos().y + l_parentTransformationMatrix[2][2] * m_transform.getPos().z + l_parentTransformationMatrix[3][2]);
}

glm::quat BaseActor::caclWorldRot()
{
	glm::quat l_parentRotationQuat = glm::quat(1, 0, 0, 0);

	if (getParentActor() != nullptr)
	{
		l_parentRotationQuat = getParentActor()->caclWorldRot();
	}

	return l_parentRotationQuat * m_transform.getRot();
}

glm::vec3 BaseActor::caclWorldScale()
{
	glm::vec3 l_parentScale = glm::vec3(1.0, 1.0, 1.0);

	if (getParentActor() != nullptr)
	{
		l_parentScale = getParentActor()->caclWorldScale();
	}

	return l_parentScale * m_transform.getScale();
}

glm::mat4 BaseActor::caclWorldPosMatrix()
{
	return glm::translate(glm::mat4(), caclWorldPos());
}

glm::mat4 BaseActor::caclWorldRotMatrix()
{
	return glm::toMat4(caclWorldRot());
}

glm::mat4 BaseActor::caclWorldScaleMatrix()
{
	return glm::scale(glm::mat4(), caclWorldScale());
}

glm::mat4 BaseActor::caclTransformationMatrix()
{
	glm::mat4 l_parentTransformationMatrix;
	
	l_parentTransformationMatrix[0][0] = 1.0f;
	l_parentTransformationMatrix[1][1] = 1.0f;
	l_parentTransformationMatrix[2][2] = 1.0f;
	l_parentTransformationMatrix[3][3] = 1.0f;

	if (getParentActor() != nullptr && getParentActor()->hasTransformChanged())
	{
		l_parentTransformationMatrix = getParentActor()->caclTransformationMatrix();
	}

	return l_parentTransformationMatrix * caclLocalPosMatrix() * caclLocalRotMatrix() * caclLocalScaleMatrix();
}

void BaseActor::initialize()
{
	for (auto l_childComponent : m_childComponents)
	{
		l_childComponent->initialize();
	}
	for (auto l_childActor : m_childActors)
	{
		l_childActor->initialize();
	}
	this->setStatus(objectStatus::ALIVE);
}

void BaseActor::update()
{
	m_transform.update();
	for (auto l_childActor : m_childActors)
	{
		l_childActor->update();
	}
}

void BaseActor::shutdown()
{
	for (auto l_childActor : m_childActors)
	{
		l_childActor->shutdown();
	}
	for (auto l_childComponent : m_childComponents)
	{
		l_childComponent->shutdown();
	}
	this->setStatus(objectStatus::SHUTDOWN);
}

BaseComponent::BaseComponent()
{
}

BaseComponent::~BaseComponent()
{
}

BaseActor* BaseComponent::getParentActor() const
{
	return m_parentActor;
}

void BaseComponent::setParentActor(BaseActor * parentActor)
{
	m_parentActor = parentActor;
}

Transform* BaseComponent::getTransform()
{
	return m_parentActor->getTransform();
}
