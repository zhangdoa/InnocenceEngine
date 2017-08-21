#include "../../main/stdafx.h"
#include "CameraComponent.h"

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

glm::mat4 CameraComponent::getTranslatonMatrix() const
{
	return glm::translate(glm::mat4(), getParentActor().caclTransformedPos() * -1.0f);
}

glm::mat4 CameraComponent::getRotationMatrix() const
{
	// quaternion rotation
	glm::quat conjugateRotQuat;
	conjugateRotQuat.w = getParentActor().caclTransformedRot().w;
	conjugateRotQuat.x = -getParentActor().caclTransformedRot().x;
	conjugateRotQuat.y = -getParentActor().caclTransformedRot().y;
	conjugateRotQuat.z = -getParentActor().caclTransformedRot().z;

	return glm::toMat4(conjugateRotQuat);
}

glm::mat4 CameraComponent::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

void CameraComponent::init()
{
	m_projectionMatrix = glm::perspective((70.0f / 180.0f) * glm::pi<float>(), (4.0f / 3.0f), 0.1f, 1000000.0f);
}

void CameraComponent::update()
{
	getTransform()->update();
	CoreManager::getInstance().getRenderingManager().setCameraProjectionMatrix(getProjectionMatrix());
	CoreManager::getInstance().getRenderingManager().setCameraTranslationMatrix(getTranslatonMatrix());
	CoreManager::getInstance().getRenderingManager().setCameraViewMatrix(getRotationMatrix());
}

void CameraComponent::shutdown()
{
}

