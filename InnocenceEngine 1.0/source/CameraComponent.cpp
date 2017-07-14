#include "stdafx.h"
#include "CameraComponent.h"

CameraData::CameraData()
{
}

CameraData::~CameraData()
{
}

void CameraData::addCameraData(float fov, float aspectRatio, float zNear, float zFar)
{
	m_projectionMatrix = glm::perspective(fov, aspectRatio, zNear, zFar);
}

glm::mat4 CameraData::getViewProjectionMatrix(BaseComponent* parent) const
{	
	return m_projectionMatrix  * getRotationMatrix(parent) * getTranslatonMatrix(parent);
}

glm::mat4 CameraData::getTranslatonMatrix(BaseComponent * parent) const
{
	// get camera's translation matrix, reverse direction to "look into"  the screen
	glm::mat4 l_cameraTranslationMatrix;
	l_cameraTranslationMatrix = glm::translate(l_cameraTranslationMatrix, parent->getParentActor().caclTransformedPos() * -1.0f);
	return l_cameraTranslationMatrix;
}

glm::mat4 CameraData::getRotationMatrix(BaseComponent * parent) const
{
	// quaternion rotation
	glm::quat conjugateRotQuat;
	conjugateRotQuat.w = parent->getParentActor().caclTransformedRot().w;
	conjugateRotQuat.x = -parent->getParentActor().caclTransformedRot().x;
	conjugateRotQuat.y = -parent->getParentActor().caclTransformedRot().y;
	conjugateRotQuat.z = -parent->getParentActor().caclTransformedRot().z;

	glm::mat4 l_cameraRotationMatrix = glm::toMat4(conjugateRotQuat);

	return l_cameraRotationMatrix;
}

glm::mat4 CameraData::getProjectionMatrix() const
{
	return m_projectionMatrix;
}

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

glm::mat4 CameraComponent::getViewProjectionMatrix()
{
	return m_cameraData.getViewProjectionMatrix(this);
}

glm::mat4 CameraComponent::getTranslatonMatrix()
{
	return m_cameraData.getTranslatonMatrix(this);
}

glm::mat4 CameraComponent::getRotationMatrix()
{
	return m_cameraData.getRotationMatrix(this);
}

glm::mat4 CameraComponent::getProjectionMatrix()
{
	return m_cameraData.getProjectionMatrix();
}

void CameraComponent::move(moveDirection moveDirection)
{
	// TODO: it should move along the direction of camera rather than the static six axis
	switch (moveDirection)
	{
	// opengl use right-hand-coordinate, so go foward means get into the negative z-axis
	case FORWARD:  getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::BACKWARD) * 0.05f); break;
	case BACKWARD:  getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::FORWARD) *  0.05f);  break;
	case LEFT:   getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::LEFT) *  0.05f);  break;
	case RIGHT:   getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::RIGHT) *  0.05f);  break;
	}

}

void CameraComponent::init()
{
	m_cameraData.addCameraData((70.0f / 180.0f) * glm::pi<float>(), (4.0f / 3.0f), 0.1f, 1000.0f);
}

void CameraComponent::update()
{
	getTransform()->update();

	if (InputManager::getInstance().getMouse(GLFW_MOUSE_BUTTON_RIGHT))
	{
		if (InputManager::getInstance().getMousePosition().x != 0)
		{
			getTransform()->rotate(glm::vec3(0.0f, 1.0f, 0.0f), ((-InputManager::getInstance().getMousePosition().x * 1.5f) / 180.0f)* glm::pi<float>());
		}
		if (InputManager::getInstance().getMousePosition().y != 0)
		{
			getTransform()->rotate(getTransform()->getDirection(Transform::RIGHT), ((InputManager::getInstance().getMousePosition().y * 1.5f) / 180.0f)* glm::pi<float>());
		}
		if (InputManager::getInstance().getMousePosition().x != 0 || InputManager::getInstance().getMousePosition().y != 0)
		{
			InputManager::getInstance().setMousePosition(glm::vec2(0.0f, 0.0f));
		}

		if (InputManager::getInstance().getKey(GLFW_KEY_W))
		{
			move(CameraComponent::FORWARD);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_S))
		{
			move(CameraComponent::BACKWARD);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_A))
		{
			move(CameraComponent::LEFT);
		}
		if (InputManager::getInstance().getKey(GLFW_KEY_D))
		{
			move(CameraComponent::RIGHT);
		}
	}
}

void CameraComponent::shutdown()
{
}

