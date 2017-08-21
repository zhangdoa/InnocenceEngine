#include "../../main/stdafx.h"
#include "InputComponent.h"


InputComponent::InputComponent()
{
}


InputComponent::~InputComponent()
{
}

void InputComponent::move(moveDirection moveDirection)
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

void InputComponent::init()
{
}

void InputComponent::update()
{
	CoreManager::getInstance().getInputManager().getMouse(GLFW_MOUSE_BUTTON_RIGHT, l_mouseRightResult);

	CoreManager::getInstance().getInputManager().getKey(GLFW_KEY_W, l_keyWResult);
	CoreManager::getInstance().getInputManager().getKey(GLFW_KEY_S, l_keySResult);
	CoreManager::getInstance().getInputManager().getKey(GLFW_KEY_A, l_keyAResult);
	CoreManager::getInstance().getInputManager().getKey(GLFW_KEY_D, l_keyDResult);

	if (l_mouseRightResult)
	{
		CoreManager::getInstance().getInputManager().getMousePosition(l_mousePosition);

		if (l_mousePosition.x != 0)
		{
			getTransform()->rotate(glm::vec3(0.0f, 1.0f, 0.0f), ((-l_mousePosition.x * rotateSpeed) / 180.0f)* glm::pi<float>());
		}
		if (l_mousePosition.y != 0)
		{
			getTransform()->rotate(getTransform()->getDirection(Transform::RIGHT), ((l_mousePosition.y * rotateSpeed) / 180.0f)* glm::pi<float>());
		}
		if (l_mousePosition.x != 0 || l_mousePosition.y != 0)
		{
			CoreManager::getInstance().getInputManager().setMousePosition(glm::vec2(0.0f, 0.0f));
		}
		if (l_keyWResult)
		{
			move(moveDirection::FORWARD);
		}
		if (l_keySResult)
		{
			move(moveDirection::BACKWARD);
		}
		if (l_keyAResult)
		{
			move(moveDirection::LEFT);
		}
		if (l_keyDResult)
		{
			move(moveDirection::RIGHT);
		}
	}
}

void InputComponent::shutdown()
{
}

