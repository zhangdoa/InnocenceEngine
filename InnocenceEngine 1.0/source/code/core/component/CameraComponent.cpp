#include "../../main/stdafx.h"
#include "CameraComponent.h"

CameraComponent::CameraComponent()
{
}

CameraComponent::~CameraComponent()
{
}

void CameraComponent::getViewProjectionMatrix(glm::mat4 & outViewProjectionMatrix) const
{
	glm::mat4 l_RotationMatrix;

	glm::mat4 l_TranslationMatrix;

	getRotationMatrix(l_RotationMatrix);

	getTranslatonMatrix(l_TranslationMatrix);

	outViewProjectionMatrix = m_projectionMatrix * l_RotationMatrix * l_TranslationMatrix;
}

void CameraComponent::getTranslatonMatrix(glm::mat4 & outTranslationMatrix) const
{
	outTranslationMatrix = glm::translate(outTranslationMatrix, getParentActor().caclTransformedPos() * -1.0f);
}

void CameraComponent::getRotationMatrix(glm::mat4 & outRotationMatrix) const
{
	// quaternion rotation
	glm::quat conjugateRotQuat;
	conjugateRotQuat.w = getParentActor().caclTransformedRot().w;
	conjugateRotQuat.x = -getParentActor().caclTransformedRot().x;
	conjugateRotQuat.y = -getParentActor().caclTransformedRot().y;
	conjugateRotQuat.z = -getParentActor().caclTransformedRot().z;

	outRotationMatrix = glm::toMat4(conjugateRotQuat);
}

void CameraComponent::getProjectionMatrix(glm::mat4 & outProjectionMatrix) const
{
	outProjectionMatrix = m_projectionMatrix;
}

void CameraComponent::move(moveDirection moveDirection)
{
	switch (moveDirection)
	{
		// opengl use right-hand-coordinate, so go foward means get into the negative z-axis
	case FORWARD:  getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::BACKWARD) * moveSpeed); break;
	case BACKWARD:  getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::FORWARD) *  moveSpeed);  break;
	case LEFT:   getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::LEFT) *  moveSpeed);  break;
	case RIGHT:   getTransform()->setPos(getTransform()->getPos() + getTransform()->getDirection(Transform::RIGHT) *  moveSpeed);  break;
	}

}

void CameraComponent::init()
{
	m_projectionMatrix = glm::perspective((70.0f / 180.0f) * glm::pi<float>(), (4.0f / 3.0f), 0.1f, 1000000.0f);
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

