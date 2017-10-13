#include "../../../main/stdafx.h"
#include "GLInputManager.h"


GLInputManager::GLInputManager()
{
}


GLInputManager::~GLInputManager()
{
}

void GLInputManager::getKey(int keyCode, int& result) const
{
	result = glfwGetKey(GLWindowManager::getInstance().getWindow(), keyCode);
}

void GLInputManager::getMouse(int mouseButton, int& result) const
{
	result = glfwGetMouseButton(GLWindowManager::getInstance().getWindow(), mouseButton);
}

void GLInputManager::getMousePosition(glm::vec2& mousePosition) const
{
	mousePosition.x = m_mouseXOffset;
	mousePosition.y = m_mouseYOffset;
}

void GLInputManager::setMousePosition(const glm::vec2 & mousePosition)
{
	m_mouseXOffset = mousePosition.x;
	m_mouseYOffset = mousePosition.y;
}

void GLInputManager::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
	getInstance().framebufferSizeCallbackImpl(window, width, height);
}

void GLInputManager::mousePositionCallback(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	getInstance().mousePositionCallbackImpl(window, mouseXPos, mouseYPos);
}

void GLInputManager::scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
}

void GLInputManager::framebufferSizeCallbackImpl(GLFWwindow * window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void GLInputManager::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	m_mouseXOffset = mouseXPos - m_mouseLastX;
	m_mouseYOffset = m_mouseLastY - mouseYPos;

	m_mouseLastX = mouseXPos;
	m_mouseLastY = mouseYPos;
}

void GLInputManager::scrollCallbackImpl(GLFWwindow * window, double xoffset, double yoffset)
{
}

void GLInputManager::initialize()
{
	std::vector<int>lastKeys(NUM_KEYCODES);
	std::vector<int>lastMouse(NUM_MOUSEBUTTONS);
	glfwSetFramebufferSizeCallback(GLWindowManager::getInstance().getWindow(), &framebufferSizeCallback);
	glfwSetCursorPosCallback(GLWindowManager::getInstance().getWindow(), &mousePositionCallback);
	glfwSetScrollCallback(GLWindowManager::getInstance().getWindow(), &scrollCallback);
	glfwSetInputMode(GLWindowManager::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_TRUE);
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("GLInputManager has been initialized.");
}

void GLInputManager::update()
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
	}
	else
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("GLInputManager is stand-by.");
	}
}

void GLInputManager::shutdown()
{
	glfwSetInputMode(GLWindowManager::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_FALSE);
	this->setStatus(objectStatus::SHUTDOWN);
	LogManager::getInstance().printLog("GLInputManager has been shutdown.");
}
