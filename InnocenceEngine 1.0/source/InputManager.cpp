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
}

void InputManager::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	if (isMouseInputFirstTime)
	{
		m_mouseLastX = mouseXPos;
		m_mouseLastY = mouseYPos;
		isMouseInputFirstTime = false;
	}

	float l_mouseXOffset = mouseXPos - m_mouseLastX;
	float l_mouseYOffset = m_mouseLastY - mouseYPos; // reversed since y-coordinates go from bottom to top

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
	//glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);
	glfwSetCursorPosCallback(WindowManager::getInstance().getWindow(), &mousePositionCallback);
	//glfwSetScrollCallback(m_window, scrollCallback);
	glfwSetInputMode(WindowManager::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_TRUE);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	this->setStatus(INITIALIZIED);
	LogManager::printLog("InputManager has been initialized.");
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
		// TODO: needs a better event-drived style input response mechanic.
		if (getKey(GLFW_KEY_ESCAPE))
		{
			fprintf(stdout, "Esc pressed.\n");
			this->setStatus(STANDBY);
			LogManager::printLog("InputManager is stand-by.");
		}
	}
	else
	{
		this->setStatus(STANDBY);
		LogManager::printLog("InputManager is stand-by.");
	}
}

void InputManager::shutdown()
{
	glfwSetInputMode(WindowManager::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_FALSE);
	this->setStatus(UNINITIALIZIED);
	LogManager::printLog("InputManager has been shutdown.");
}
