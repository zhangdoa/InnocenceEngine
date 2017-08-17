#include "../../main/stdafx.h"
#include "InputComponent.h"


InputComponent::InputComponent()
{
}


InputComponent::~InputComponent()
{
}

void InputComponent::init()
{
}

void InputComponent::update()
{
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
	}
}

void InputComponent::shutdown()
{
}

