#include "../../../main/stdafx.h"
#include "GLInputManager.h"


GLInputManager::GLInputManager()
{
}


GLInputManager::~GLInputManager()
{
}

void GLInputManager::addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback)
{
	auto l_keyboardInputCallbackFunctionVector = m_keyboardInputCallback.find(keyCode);
	if (l_keyboardInputCallbackFunctionVector != m_keyboardInputCallback.end())
	{
		l_keyboardInputCallbackFunctionVector->second.emplace_back(keyboardInputCallback);
	}
	else
	{
		m_keyboardInputCallback.emplace(keyCode, std::vector<std::function<void()>*>{keyboardInputCallback});
	}
}

void GLInputManager::addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(keyCode, i);
	}
}

void GLInputManager::addKeyboardInputCallback(std::multimap<int, std::vector<std::function<void()>*>>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(i.first, i.second);
	}
}

void GLInputManager::addMouseMovementCallback(int keyCode, std::function<void(float)>* mouseMovementCallback)
{
	auto l_mouseMovementCallbackFunctionVector = m_mouseMovementCallback.find(keyCode);
	if (l_mouseMovementCallbackFunctionVector != m_mouseMovementCallback.end())
	{
		l_mouseMovementCallbackFunctionVector->second.emplace_back(mouseMovementCallback);
	}
	else
	{
		m_mouseMovementCallback.emplace(keyCode, std::vector<std::function<void(float)>*>{mouseMovementCallback});
	}
}

void GLInputManager::addMouseMovementCallback(int keyCode, std::vector<std::function<void(float)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(keyCode, i);
	}
}

void GLInputManager::addMouseMovementCallback(std::multimap<int, std::vector<std::function<void(float)>*>>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(i.first, i.second);
	}
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
				auto l_keybinding = m_keyboardInputCallback.find(i);
				if (l_keybinding != m_keyboardInputCallback.end())
				{
					for (auto j : l_keybinding->second)
					{
						if (j)
						{
							(*j)();
						}
					}
				}
			}
		}


		if (glfwGetMouseButton(GLWindowManager::getInstance().getWindow(), 1) == GLFW_PRESS)
		{
			GLWindowManager::getInstance().hideMouseCursor();
			if (m_mouseXOffset != 0)
			{
				for (auto j : m_mouseMovementCallback.find(0)->second)
				{
					(*j)(m_mouseXOffset);
				};
			}
			if (m_mouseYOffset != 0)
			{
				for (auto j : m_mouseMovementCallback.find(1)->second)
				{
					(*j)(m_mouseYOffset);
				};
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
