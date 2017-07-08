#include "stdafx.h"
#include "IGameEntity.h"

Transform::Transform()
{
	_pos = glm::vec3(0.0f, 0.0f, 0.0f);
	_rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	_scale = glm::vec3(1.0f, 1.0f, 1.0f);
	_oldPos = _pos + (1.0f);
	_oldRot = _rot *(0.5f);
	_oldScale = _scale + (1.0f);
}

Transform::~Transform()
{
}

void Transform::update()
{
	_oldPos = _pos;
	_oldRot = _rot;
	_oldScale = _scale;
}

void Transform::rotate(glm::vec3 axis, float angle)
{
	float sinHalfAngle = sinf(angle / 2);
	float cosHalfAngle = cosf(angle / 2);
	glm::quat rotateFactor;
	rotateFactor.x = axis.x * sinHalfAngle;
	rotateFactor.y = axis.y * sinHalfAngle;
	rotateFactor.z = axis.z * sinHalfAngle;
	rotateFactor.w = cosHalfAngle;
	_rot = (glm::normalize(rotateFactor * _rot));
}

const glm::vec3 & Transform::getPos()
{
	return _pos;
}

const glm::quat & Transform::getRot()
{
	return _rot;
}

const glm::vec3 & Transform::getScale()
{
	return _scale;
}

void Transform::setPos(const glm::vec3 & pos)
{
	_pos = pos;
}

void Transform::setRot(const glm::quat & rot)
{
	_rot = rot;
}

void Transform::setScale(const glm::vec3 & scale)
{
	_scale = scale;
}

const glm::vec3 & Transform::getOldPos()
{
	return _oldPos;
}

const glm::quat & Transform::getOldRot()
{
	return _oldRot;
}

const glm::vec3 & Transform::getOldScale()
{
	return _oldScale;
}

glm::vec3 Transform::getForward() const
{
	glm::quat t;
	glm::vec3 r = glm::vec3(0, 0, 1);
	t.w = -_rot.x * r.x - _rot.y * r.y - _rot.z * r.z;
	t.x = _rot.w * r.x + _rot.y * r.z - _rot.z * r.y;
	t.y = _rot.w * r.y + _rot.z * r.x - _rot.x * r.z;
	t.z = _rot.w * r.z + _rot.x * r.y - _rot.y * r.x;
	t = _rot * t * -1.0f;
	return 	glm::vec3(t.x, t.y, t.z);
}

glm::vec3 Transform::getBackward() const
{
	glm::quat t;
	glm::vec3 r = glm::vec3(0, 0, -1);
	t.w = -_rot.x * r.x - _rot.y * r.y - _rot.z * r.z;
	t.x = _rot.w * r.x + _rot.y * r.z - _rot.z * r.y;
	t.y = _rot.w * r.y + _rot.z * r.x - _rot.x * r.z;
	t.z = _rot.w * r.z + _rot.x * r.y - _rot.y * r.x;
	t = _rot * t * -1.0f;
	return 	glm::vec3(t.x, t.y, t.z);
}

glm::vec3 Transform::getUp() const
{
	glm::quat t;
	glm::vec3 r = glm::vec3(0, 1, 0);
	t.w = -_rot.x * r.x - _rot.y * r.y - _rot.z * r.z;
	t.x = _rot.w * r.x + _rot.y * r.z - _rot.z * r.y;
	t.y = _rot.w * r.y + _rot.z * r.x - _rot.x * r.z;
	t.z = _rot.w * r.z + _rot.x * r.y - _rot.y * r.x;
	t = _rot * t * -1.0f;
	return 	glm::vec3(t.x, t.y, t.z);
}

glm::vec3 Transform::getDown() const
{
	glm::quat t;
	glm::vec3 r = glm::vec3(0, -1, 0);
	t.w = -_rot.x * r.x - _rot.y * r.y - _rot.z * r.z;
	t.x = _rot.w * r.x + _rot.y * r.z - _rot.z * r.y;
	t.y = _rot.w * r.y + _rot.z * r.x - _rot.x * r.z;
	t.z = _rot.w * r.z + _rot.x * r.y - _rot.y * r.x;
	t = _rot * t * -1.0f;
	return 	glm::vec3(t.x, t.y, t.z);
}

glm::vec3 Transform::getRight() const
{ 
	glm::quat t;
	glm::vec3 r = glm::vec3(1, 0, 0);
	t.w = -_rot.x * r.x - _rot.y * r.y - _rot.z * r.z;
	t.x = _rot.w * r.x + _rot.y * r.z - _rot.z * r.y;
	t.y = _rot.w * r.y + _rot.z * r.x - _rot.x * r.z;
	t.z = _rot.w * r.z + _rot.x * r.y - _rot.y * r.x;
	t = _rot * t * -1.0f;
	return 	glm::vec3(t.x, t.y, t.z);
}

glm::vec3 Transform::getLeft() const
{
	glm::quat t;
	glm::vec3 r = glm::vec3(-1, 0, 0);
	t.w = -_rot.x * r.x - _rot.y * r.y - _rot.z * r.z;
	t.x = _rot.w * r.x + _rot.y * r.z - _rot.z * r.y;
	t.y = _rot.w * r.y + _rot.z * r.x - _rot.x * r.z;
	t.z = _rot.w * r.z + _rot.x * r.y - _rot.y * r.x;
	t = _rot * t * -1.0f;
	return 	glm::vec3(t.x, t.y, t.z);
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

glm::mat4 BaseActor::caclTransformation()
{
	glm::mat4 l_translationMatrix;

	l_translationMatrix[0][0] = 1.0f;
	l_translationMatrix[0][1] = 0.0f;
	l_translationMatrix[0][2] = 0.0f;
	l_translationMatrix[0][3] = m_transform.getPos().x;
	l_translationMatrix[1][0] = 0.0f;
	l_translationMatrix[1][1] = 1.0f;
	l_translationMatrix[1][2] = 0.0f;
	l_translationMatrix[1][3] = m_transform.getPos().y;
	l_translationMatrix[2][0] = 0.0f;
	l_translationMatrix[2][1] = 0.0f;
	l_translationMatrix[2][2] = 1.0f;
	l_translationMatrix[2][3] = m_transform.getPos().z;
	l_translationMatrix[3][0] = 0.0f;
	l_translationMatrix[3][1] = 0.0f;
	l_translationMatrix[3][2] = 0.0f;
	l_translationMatrix[3][3] = 1.0f;

	glm::mat4 l_rotaionMartix = m_transform.QuatToRotationMatrix(m_transform.getRot());

	glm::mat4 l_scaleMartix;

	l_scaleMartix[0][0] = m_transform.getScale().x;
	l_scaleMartix[0][1] = 0.0f;
	l_scaleMartix[0][2] = 0.0f;
	l_scaleMartix[0][3] = 0.0f;
	l_scaleMartix[1][0] = 0.0f;
	l_scaleMartix[1][1] = m_transform.getScale().y;
	l_scaleMartix[1][2] = 0.0f;
	l_scaleMartix[1][3] = 0.0f;
	l_scaleMartix[2][0] = 0.0f;
	l_scaleMartix[2][1] = 0.0f;
	l_scaleMartix[2][2] = m_transform.getScale().z;
	l_scaleMartix[2][3] = 0.0f;
	l_scaleMartix[3][0] = 0.0f;
	l_scaleMartix[3][1] = 0.0f;
	l_scaleMartix[3][2] = 0.0f;
	l_scaleMartix[3][3] = 1.0f;

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

	if (getParentActor() != nullptr && getParentActor()->hasTransformChanged())
	{
		l_parentMatrix = getParentActor()->caclTransformation();
	}

	return l_scaleMartix * l_rotaionMartix * l_translationMatrix * l_parentMatrix;
}

glm::vec3 BaseActor::caclTransformedPos()
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

	if (getParentActor() != nullptr && getParentActor()->hasTransformChanged())
	{
		l_parentMatrix = getParentActor()->caclTransformation();
	}

	return glm::vec3(l_parentMatrix[0][0] * m_transform.getPos().x + l_parentMatrix[0][1] * m_transform.getPos().y + l_parentMatrix[0][2] * m_transform.getPos().z + l_parentMatrix[0][3],
		l_parentMatrix[1][0] * m_transform.getPos().x + l_parentMatrix[1][1] * m_transform.getPos().y + l_parentMatrix[1][2] * m_transform.getPos().z + l_parentMatrix[1][3],
		l_parentMatrix[2][0] * m_transform.getPos().x + l_parentMatrix[2][1] * m_transform.getPos().y + l_parentMatrix[2][2] * m_transform.getPos().z + l_parentMatrix[2][3]);
}

glm::quat BaseActor::caclTransformedRot()
{
	glm::quat l_parentRotation = glm::quat(1, 0, 0, 0);

	if (getParentActor() != nullptr)
	{
		l_parentRotation = getParentActor()->caclTransformedRot();
	}

	return m_transform.getRot() * l_parentRotation;
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
