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
	float tanHalfFOV = tanf(fov / 2);
	float zRange = zNear - zFar;

	m_projectionMatrix[0][0] = 1.0f / (tanHalfFOV * aspectRatio);
	m_projectionMatrix[0][1] = 0.0f;
	m_projectionMatrix[0][2] = 0.0f;
	m_projectionMatrix[0][3] = 0.0f;
	m_projectionMatrix[1][0] = 0.0f;
	m_projectionMatrix[1][1] = 1.0f / tanHalfFOV;
	m_projectionMatrix[1][2] = 0.0f;
	m_projectionMatrix[1][3] = 0.0f;
	m_projectionMatrix[2][0] = 0.0f;
	m_projectionMatrix[2][1] = 0.0f;
	m_projectionMatrix[2][2] = (-zNear - zFar) / zRange;
	m_projectionMatrix[2][3] = 2 * zFar * zNear / zRange;
	m_projectionMatrix[3][0] = 0.0f;
	m_projectionMatrix[3][1] = 0.0f;
	m_projectionMatrix[3][2] = 1.0f;
	m_projectionMatrix[3][3] = 0.0f;
}

glm::mat4 CameraData::getViewProjectionMatrix(BaseComponent* parent) const
{
	// get camera's translation matrix, reverse direction to "look into"  the screen
	glm::mat4 l_cameraTranslationMatrix;
	l_cameraTranslationMatrix = glm::translate(l_cameraTranslationMatrix, parent->getParentActor().caclTransformedPos() * -1.0f);

	// quaternion rotation
	glm::quat conjugateRotQuat;
	conjugateRotQuat.w = parent->getParentActor().caclTransformedRot().w;
	conjugateRotQuat.x = -parent->getParentActor().caclTransformedRot().x;
	conjugateRotQuat.y = -parent->getParentActor().caclTransformedRot().y;
	conjugateRotQuat.z = -parent->getParentActor().caclTransformedRot().z;

	glm::mat4 l_cameraRotationMatrix = parent->getTransform()->QuatToRotationMatrix(conjugateRotQuat);

	return m_projectionMatrix  * l_cameraRotationMatrix * l_cameraTranslationMatrix;
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

void CameraComponent::move(moveDirection moveDirection)
{
	// TODO: it should move along the direction of camera rather than the static six axis
	switch (moveDirection)
	{
	// opengl use right-hand-coordinate, so go foward means get into the negative z-axis
	case FORWARD:  getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::BACKWARD) * 0.03f); break;
	case BACKWARD:  getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::FORWARD) *  0.03f);  break;
	case LEFT:   getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::LEFT) *  0.03f);  break;
	case RIGHT:   getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::RIGHT) *  0.03f);  break;
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
		// TODO: if move mouse with small scale it would cause reverse movement
		glm::vec2 deltaPos = InputManager::getInstance().getMousePosition() - WindowManager::getInstance().getScreenCenterPosition();
		if (deltaPos.x != 0)
		{
			getTransform()->rotate(glm::vec3(0.0f, 1.0f, 0.0f), ((deltaPos.x * 0.05f) / 180.0f)* glm::pi<float>());
		}
		if (deltaPos.y != 0)
		{
			getTransform()->rotate(getTransform()->getDirection(Transform::RIGHT), ((-deltaPos.y * 0.05f) / 180.0f)* glm::pi<float>());
		}
		if (deltaPos.x != 0 || deltaPos.y != 0)
		{
			InputManager::getInstance().setMousePosition(WindowManager::getInstance().getScreenCenterPosition());
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

