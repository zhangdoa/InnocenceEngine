#include "stdafx.h"
#include "InputManager.h"


InputManager::InputManager()
{
}


InputManager::~InputManager()
{
}

int InputManager::getKey(int keyCode) const
{
	return glfwGetKey(WindowManager::getInstance().getWindow(), keyCode);
}

int InputManager::getMouse(int mouseButton) const
{
	return glfwGetMouseButton(WindowManager::getInstance().getWindow(), mouseButton);
}

glm::vec2 InputManager::getMousePosition() const
{
	return glm::vec2(m_mouseLastX, m_mouseLastY);
}

void InputManager::setMousePosition(const glm::vec2 & mousePosition)
{
	m_mouseLastX = mousePosition.x;
	m_mouseLastY = mousePosition.y;
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
	glfwSetFramebufferSizeCallback(WindowManager::getInstance().getWindow(), &framebufferSizeCallback);
	glfwSetCursorPosCallback(WindowManager::getInstance().getWindow(), &mousePositionCallback);
	glfwSetScrollCallback(WindowManager::getInstance().getWindow(), &scrollCallback);
	glfwSetInputMode(WindowManager::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_TRUE);
	this->setStatus(INITIALIZIED);
	LogManager::getInstance().printLog("InputManager has been initialized.");
}

void InputManager::update()
{
	if (WindowManager::getInstance().getWindow() != nullptr)
	{
		m_lastKeys.clear();
		for (int i = 0; i < NUM_KEYCODES; i++)
		{
			m_lastKeys.emplace_back(getKey(i));
		}

		m_lastMouse.clear();
		for (int i = 0; i < NUM_KEYCODES; i++)
		{
			m_lastMouse.emplace_back(getMouse(i));
		}
		if (glfwGetMouseButton(WindowManager::getInstance().getWindow(), GLFW_MOUSE_BUTTON_RIGHT))
		{
			glfwSetInputMode(WindowManager::getInstance().getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		if (getKey(GLFW_KEY_ESCAPE))
		{
			if (glfwGetInputMode(WindowManager::getInstance().getWindow(), GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
			{
				glfwSetInputMode(WindowManager::getInstance().getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
		}
	}
	else
	{
		this->setStatus(STANDBY);
		LogManager::getInstance().printLog("InputManager is stand-by.");
	}
}

void InputManager::shutdown()
{
	glfwSetInputMode(WindowManager::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_FALSE);
	this->setStatus(UNINITIALIZIED);
	LogManager::getInstance().printLog("InputManager has been shutdown.");
}
