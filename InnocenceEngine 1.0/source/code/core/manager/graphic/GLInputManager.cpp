#include "../../../main/stdafx.h"
#include "GLInputManager.h"


GLInputManager::GLInputManager()
{
}


GLInputManager::~GLInputManager()
{
}

void GLInputManager::setKeyboardInputCallback(std::multimap<int, std::function<void()>>& keyboardInputCallback)
{
	m_keyboardInputCallback = keyboardInputCallback;
}

void GLInputManager::setMouseMovementCallback(std::multimap<int, std::function<void(float)>>& mouseMovementCallback)
{
	m_mouseMovementCallback = mouseMovementCallback;
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

		for (int i = 0; i < NUM_KEYCODES; i++)
		{
			if (glfwGetKey(GLWindowManager::getInstance().getWindow(), i) == GLFW_PRESS)
			{
				m_keyboardInputCallback.find(i)->second();
			}
		}


		if (glfwGetMouseButton(GLWindowManager::getInstance().getWindow(), 1) == GLFW_PRESS)
		{
			GLWindowManager::getInstance().hideMouseCursor();
			if (m_mouseXOffset != 0)
			{
				m_mouseMovementCallback.find(0)->second(m_mouseXOffset);
			}
			if (m_mouseYOffset != 0)
			{
				m_mouseMovementCallback.find(1)->second(m_mouseYOffset);
			}
			if (m_mouseXOffset != 0 || m_mouseYOffset != 0)
			{
				m_mouseXOffset = 0;
				m_mouseYOffset = 0;
			}
		}
		else
		{
			GLWindowManager::getInstance().showMouseCursor();
		}
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
