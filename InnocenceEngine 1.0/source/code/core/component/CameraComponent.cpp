#include "../../main/stdafx.h"
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

void CameraData::getViewProjectionMatrix(const BaseComponent& parent, glm::mat4 & outViewProjectionMatrix) const
{
	glm::mat4 l_RotationMatrix;

	glm::mat4 l_TranslationMatrix;

	getRotationMatrix(parent, l_RotationMatrix);

	getTranslatonMatrix(parent, l_TranslationMatrix);

	outViewProjectionMatrix = m_projectionMatrix * l_RotationMatrix * l_TranslationMatrix;
}

void CameraData::getTranslatonMatrix(const BaseComponent& parent, glm::mat4 & outTranslationMatrix) const
{
	// get camera's translation matrix, reverse direction to "look into"  the screen
	outTranslationMatrix = glm::translate(outTranslationMatrix, parent.getParentActor().caclTransformedPos() * -1.0f);
}

void CameraData::getRotationMatrix(const BaseComponent& parent, glm::mat4 & outRotationMatrix) const
{
	// quaternion rotation
	glm::quat conjugateRotQuat;
	conjugateRotQuat.w = parent.getParentActor().caclTransformedRot().w;
	conjugateRotQuat.x = -parent.getParentActor().caclTransformedRot().x;
	conjugateRotQuat.y = -parent.getParentActor().caclTransformedRot().y;
	conjugateRotQuat.z = -parent.getParentActor().caclTransformedRot().z;

	outRotationMatrix = glm::toMat4(conjugateRotQuat);
}

void CameraData::getProjectionMatrix(glm::mat4 & outProjectionMatrix) const
{
	outProjectionMatrix = m_projectionMatrix;
}

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::getViewProjectionMatrix(glm::mat4 & outViewProjectionMatrix) const
{
	m_cameraData.getViewProjectionMatrix(*this, outViewProjectionMatrix);
}

void CameraComponent::getTranslatonMatrix(glm::mat4 & outTranslationMatrix) const
{
	m_cameraData.getTranslatonMatrix(*this, outTranslationMatrix);
}

void CameraComponent::getRotationMatrix(glm::mat4 & outRotationMatrix) const
{
	m_cameraData.getRotationMatrix(*this, outRotationMatrix);
}

void CameraComponent::getProjectionMatrix(glm::mat4 & outProjectionMatrix) const
{
	m_cameraData.getProjectionMatrix(outProjectionMatrix);
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

	InputManager::getInstance().getMouse(GLFW_MOUSE_BUTTON_RIGHT, l_mouseRightResult);

	if (l_mouseRightResult)
	{
		InputManager::getInstance().getMousePosition(l_mousePosition);

		if (l_mousePosition.x != 0)
		{
			getTransform()->rotate(glm::vec3(0.0f, 1.0f, 0.0f), ((-l_mousePosition.x * 1.5f) / 180.0f)* glm::pi<float>());
		}
		if (l_mousePosition.y != 0)
		{
			getTransform()->rotate(getTransform()->getDirection(Transform::RIGHT), ((l_mousePosition.y * 1.5f) / 180.0f)* glm::pi<float>());
		}
		if (l_mousePosition.x != 0 || l_mousePosition.y != 0)
		{
			InputManager::getInstance().setMousePosition(glm::vec2(0.0f, 0.0f));
		}


		InputManager::getInstance().getKey(GLFW_KEY_W, l_keyWResult);

		InputManager::getInstance().getKey(GLFW_KEY_S, l_keySResult);

		InputManager::getInstance().getKey(GLFW_KEY_A, l_keyAResult);

		InputManager::getInstance().getKey(GLFW_KEY_D, l_keyDResult);

		if (l_keyWResult)
		{
			move(CameraComponent::FORWARD);
		}
		if (l_keySResult)
		{
			move(CameraComponent::BACKWARD);
		}
		if (l_keyAResult)
		{
			move(CameraComponent::LEFT);
		}
		if (l_keyDResult)
		{
			move(CameraComponent::RIGHT);
		}
	}
}

void CameraComponent::shutdown()
{
}

