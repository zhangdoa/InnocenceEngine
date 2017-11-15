#include "../../main/stdafx.h"
#include "InputComponent.h"


InputComponent::InputComponent()
{
}


InputComponent::~InputComponent()
{
}


void InputComponent::registerInputCallback(int keyCode, void * function)
{
	// @TODO: it seems that there should be a template for unknown member function 
}

std::multimap<int, std::function<void()>>& InputComponent::getKeyboardInputCallbackImpl()
{
	return m_keyboardInputCallbackImpl;
}

std::multimap<int, std::function<void(float)>>& InputComponent::getMouseInputCallbackImpl()
{
	return m_mouseMovementCallbackImpl;
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

void InputComponent::rotateAroundXAxis(float offset)
{
	getTransform()->rotate(glm::vec3(0.0f, 1.0f, 0.0f), ((-offset * rotateSpeed) / 180.0f)* glm::pi<float>());
}

void InputComponent::rotateAroundYAxis(float offset)
{
	getTransform()->rotate(getTransform()->getDirection(Transform::RIGHT), ((offset * rotateSpeed) / 180.0f)* glm::pi<float>());
}

void InputComponent::initialize()
{
	std::function<void()> f_moveForward = std::bind(&InputComponent::moveForward, this);
	std::function<void()> f_moveBackward = std::bind(&InputComponent::moveBackward, this);
	std::function<void()> f_moveLeft = std::bind(&InputComponent::moveLeft, this);
	std::function<void()> f_moveRight = std::bind(&InputComponent::moveRight, this);
	std::function<void(float)> f_rotateAroundXAxis = std::bind(&InputComponent::rotateAroundXAxis, this, std::placeholders::_1);
	std::function<void(float)> f_rotateAroundYAxis = std::bind(&InputComponent::rotateAroundYAxis, this, std::placeholders::_1);

	m_keyboardInputCallbackImpl.insert(std::pair<int, std::function<void()>>(GLFW_KEY_W, f_moveForward));
	m_keyboardInputCallbackImpl.insert(std::pair<int, std::function<void()>>(GLFW_KEY_S, f_moveBackward));
	m_keyboardInputCallbackImpl.insert(std::pair<int, std::function<void()>>(GLFW_KEY_A, f_moveLeft));
	m_keyboardInputCallbackImpl.insert(std::pair<int, std::function<void()>>(GLFW_KEY_D, f_moveRight));

	// @TODO: key name binding
	m_mouseMovementCallbackImpl.insert(std::pair<int, std::function<void(float)>>(0, f_rotateAroundXAxis));
	m_mouseMovementCallbackImpl.insert(std::pair<int, std::function<void(float)>>(1, f_rotateAroundYAxis));
}

void InputComponent::update()
{
	//CoreManager::getInstance().getRenderingManager().getInputManager().getMouse(GLFW_MOUSE_BUTTON_RIGHT, l_mouseRightResult);

	//CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_ESCAPE, l_ketEscapeResult);
	//CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_F1, l_keyF1Result);
	//CoreManager::getInstance().getRenderingManager().getInputManager().getKey(GLFW_KEY_F2, l_keyF2Result);

	if (l_mouseRightResult)
	{
		//CoreManager::getInstance().getRenderingManager().getWindowManager().hideMouseCursor();
		if (l_mousePosition.x != 0 || l_mousePosition.y != 0)
		{
			//CoreManager::getInstance().getRenderingManager().getInputManager().setMousePosition(glm::vec2(0.0f, 0.0f));
		}
	}
	else
	{
		//CoreManager::getInstance().getRenderingManager().getWindowManager().showMouseCursor();
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
			//CoreManager::getInstance().getRenderingManager().changeDrawPolygonMode();
		}
	}
	if (l_keyF2Result != l_oldKeyF2Result)
	{
		l_oldKeyF2Result = l_keyF2Result;
		if (l_keyF2Result)
		{
			//CoreManager::getInstance().getRenderingManager().toggleDepthBufferVisualizer();
		}
	}
}

void InputComponent::shutdown()
{
}

void InputComponent::moveForward()
{
	move(moveDirection::FORWARD);
}

void InputComponent::moveBackward()
{
	move(moveDirection::BACKWARD);
}

void InputComponent::moveLeft()
{
	move(moveDirection::LEFT);
}

void InputComponent::moveRight()
{
	move(moveDirection::RIGHT);
}

