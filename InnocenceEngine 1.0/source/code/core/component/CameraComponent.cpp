#include "../../main/stdafx.h"
#include "CameraComponent.h"

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

glm::mat4 CameraComponent::getPosMatrix() const
{
	return glm::translate(glm::mat4(), getParentActor()->caclWorldPos() * -1.0f);
}

glm::mat4 CameraComponent::getRotMatrix() const
{
	// quaternion rotation
	glm::quat conjugateRotQuat;
	conjugateRotQuat.w = getParentActor()->caclWorldRot().w;
	conjugateRotQuat.x = -getParentActor()->caclWorldRot().x;
	conjugateRotQuat.y = -getParentActor()->caclWorldRot().y;
	conjugateRotQuat.z = -getParentActor()->caclWorldRot().z;

	return glm::toMat4(conjugateRotQuat);
}

glm::mat4 CameraComponent::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

void CameraComponent::setup()
{
	IEntity::setup();
}

void CameraComponent::initialize()
{
	m_projectionMatrix = glm::perspective((70.0f / 180.0f) * glm::pi<float>(), (4.0f / 3.0f), 0.1f, 1000000.0f);
}

void CameraComponent::update()
{
}

void CameraComponent::shutdown()
{
}

