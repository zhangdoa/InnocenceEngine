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

void InputComponent::initialize()
{
}

void InputComponent::update()
{
	CoreManager::getInstance().getRenderingManager().getInputManager().getMouse(GLFW_MOUSE_BUTTON_RIGHT, l_mouseRightResult);

	CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_W, l_keyWResult);
	CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_S, l_keySResult);
	CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_A, l_keyAResult);
	CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_D, l_keyDResult);
	CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_ESCAPE, l_ketEscapeResult);
	CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_F1, l_keyF1Result);
	CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_F2, l_keyF2Result);

	if (l_mouseRightResult)
	{
		CoreManager::getInstance().getRenderingManager().getWindowManager().hideMouseCursor();
		CoreManager::getInstance().getRenderingManager().getInputManager().getMousePosition(l_mousePosition);

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
			CoreManager::getInstance().getRenderingManager().getInputManager().setMousePosition(glm::vec2(0.0f, 0.0f));
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
	else
	{
		CoreManager::getInstance().getRenderingManager().getWindowManager().showMouseCursor();
	}
	if (l_ketEscapeResult)
	{
	// @Placeholder
	}
	if (l_keyF1Result != l_oldKeyF1Result)
	{
		l_oldKeyF1Result = l_keyF1Result;
		if (l_keyF1Result)
		{
			CoreManager::getInstance().getRenderingManager().changeDrawPolygonMode();
		}	
	}
	if (l_keyF2Result != l_oldKeyF2Result)
	{
		l_oldKeyF2Result = l_keyF2Result;
		if (l_keyF2Result)
		{
			CoreManager::getInstance().getRenderingManager().toggleDepthBufferVisualizer();
		}
	}
}

void InputComponent::shutdown()
{
}

