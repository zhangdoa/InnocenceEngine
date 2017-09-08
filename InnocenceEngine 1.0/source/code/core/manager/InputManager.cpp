#include "../../main/stdafx.h"
#include "InputManager.h"


InputManager::InputManager()
{
}


InputManager::~InputManager()
{
}

void InputManager::getKey(int keyCode, int& result) const
{
	result = glfwGetKey(GLWindowManager::getInstance().getWindow(), keyCode);
}

void InputManager::getMouse(int mouseButton, int& result) const
{
	result = glfwGetMouseButton(GLWindowManager::getInstance().getWindow(), mouseButton);
}

void InputManager::getMousePosition(glm::vec2& mousePosition) const
{
	mousePosition.x = m_mouseXOffset;
	mousePosition.y = m_mouseYOffset;
}

void InputManager::setMousePosition(const glm::vec2 & mousePosition)
{
	m_mouseXOffset = mousePosition.x;
	m_mouseYOffset = mousePosition.y;
}

void InputManager::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
	getInstance().framebufferSizeCallbackImpl(window, width, height);
}

void InputManager::mousePositionCallback(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	getInstance().mousePositionCallbackImpl(window, mouseXPos, mouseYPos);
}

void InputManager::scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
}

void InputManager::framebufferSizeCallbackImpl(GLFWwindow * window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void InputManager::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	m_mouseXOffset = mouseXPos - m_mouseLastX;
	m_mouseYOffset = m_mouseLastY - mouseYPos;

	m_mouseLastX = mouseXPos;
	m_mouseLastY = mouseYPos;
}

void InputManager::scrollCallbackImpl(GLFWwindow * window, double xoffset, double yoffset)
{
}

void InputManager::init()
{
	std::vector<int>lastKeys(NUM_KEYCODES);
	std::vector<int>lastMouse(NUM_MOUSEBUTTONS);
	glfwSetFramebufferSizeCallback(GLWindowManager::getInstance().getWindow(), &framebufferSizeCallback);
	glfwSetCursorPosCallback(GLWindowManager::getInstance().getWindow(), &mousePositionCallback);
	glfwSetScrollCallback(GLWindowManager::getInstance().getWindow(), &scrollCallback);
	glfwSetInputMode(GLWindowManager::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_TRUE);
	this->setStatus(objectStatus::INITIALIZIED);
	LogManager::getInstance().printLog("InputManager has been initialized.");
}

void InputManager::update()
{
	if (GLWindowManager::getInstance().getWindow() != nullptr)
	{
		//m_lastKeys.clear();
		//for (int i = 0; i < NUM_KEYCODES; i++)
		//{
		//	m_lastKeys.emplace_back(getKey(i));
		//}

		//m_lastMouse.clear();
		//for (int i = 0; i < NUM_KEYCODES; i++)
		//{
		//	m_lastMouse.emplace_back(getMouse(i));
		//}
		if (glfwGetMouseButton(GLWindowManager::getInstance().getWindow(), GLFW_MOUSE_BUTTON_RIGHT))
		{
			glfwSetInputMode(GLWindowManager::getInstance().getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}

		int l_escapeResult = 0;

		getKey(GLFW_KEY_ESCAPE, l_escapeResult);

		if (l_escapeResult)
		{
			if (glfwGetInputMode(GLWindowManager::getInstance().getWindow(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
			{
				glfwSetInputMode(GLWindowManager::getInstance().getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
	}
	else
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("InputManager is stand-by.");
	}
}

void InputManager::shutdown()
{
	glfwSetInputMode(GLWindowManager::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_FALSE);
	this->setStatus(objectStatus::UNINITIALIZIED);
	LogManager::getInstance().printLog("InputManager has been shutdown.");
}
