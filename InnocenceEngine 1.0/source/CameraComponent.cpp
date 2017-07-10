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
	m_projectionMatrix[3][2] = 0.0f;
	m_projectionMatrix[3][3] = 0.0f;
}

glm::mat4 CameraData::getViewProjectionMatrix(BaseComponent* parent)
{
	glm::mat4 l_cameraRotationMatrix = parent->getTransform()->QuatToRotationMatrix(parent->getParentActor()->caclTransformedRot() * -1.0f);

	glm::vec3 l_cameraPosition = parent->getParentActor()->caclTransformedPos() * -1.0f;

	glm::mat4 l_cameraTranslationMatrix;

	l_cameraTranslationMatrix[0][0] = 1.0f;
	l_cameraTranslationMatrix[0][1] = 0.0f;
	l_cameraTranslationMatrix[0][2] = 0.0f;
	l_cameraTranslationMatrix[0][3] = l_cameraPosition.x;
	l_cameraTranslationMatrix[1][0] = 0.0f;
	l_cameraTranslationMatrix[1][1] = 1.0f;
	l_cameraTranslationMatrix[1][2] = 0.0f;
	l_cameraTranslationMatrix[1][3] = l_cameraPosition.y;
	l_cameraTranslationMatrix[2][0] = 0.0f;
	l_cameraTranslationMatrix[2][1] = 0.0f;
	l_cameraTranslationMatrix[2][2] = 1.0f;
	l_cameraTranslationMatrix[2][3] = l_cameraPosition.z;
	l_cameraTranslationMatrix[3][0] = 0.0f;
	l_cameraTranslationMatrix[3][1] = 0.0f;
	l_cameraTranslationMatrix[3][2] = 0.0f;
	l_cameraTranslationMatrix[3][3] = 1.0f;

	return m_projectionMatrix  * l_cameraTranslationMatrix;//* l_cameraRotationMatrix * ;
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
	switch (moveDirection)
	{
	case FORWARD:  this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getForward() * 3.0f); break;
	case BACKWARD:  this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getBackward() * 3.0f);  break;
	case LEFT:   this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getLeft() * 3.0f);  break;
	case RIGHT:   this->getTransform()->setPos(this->getTransform()->getPos() + this->getTransform()->getRight() * 3.0f);  break;
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
		glm::vec2 deltaPos = InputManager::getInstance().getMousePosition() - WindowManager::getInstance().getScreenCenterPosition();
		if (deltaPos.x != 0)
		{
			getTransform()->rotate(this->getTransform()->getUp(), ((deltaPos.x * 0.0001f) / 180.0f)* glm::pi<float>());
		}
		if (deltaPos.y != 0)
		{
			getTransform()->rotate(this->getTransform()->getRight(), ((-deltaPos.y * 0.0001f) / 180.0f)* glm::pi<float>());
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

