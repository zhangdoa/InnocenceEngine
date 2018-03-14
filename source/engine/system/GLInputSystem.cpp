#include "GLInputSystem.h"

void GLInputSystem::addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback)
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

void GLInputSystem::addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(keyCode, i);
	}
}

void GLInputSystem::addKeyboardInputCallback(std::multimap<int, std::vector<std::function<void()>*>>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(i.first, i.second);
	}
}

void GLInputSystem::addMouseMovementCallback(int keyCode, std::function<void(double)>* mouseMovementCallback)
{
	auto l_mouseMovementCallbackFunctionVector = m_mouseMovementCallback.find(keyCode);
	if (l_mouseMovementCallbackFunctionVector != m_mouseMovementCallback.end())
	{
		l_mouseMovementCallbackFunctionVector->second.emplace_back(mouseMovementCallback);
	}
	else
	{
		m_mouseMovementCallback.emplace(keyCode, std::vector<std::function<void(double)>*>{mouseMovementCallback});
	}
}

void GLInputSystem::addMouseMovementCallback(int keyCode, std::vector<std::function<void(double)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(keyCode, i);
	}
}

void GLInputSystem::addMouseMovementCallback(std::multimap<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(i.first, i.second);
	}
}

void GLInputSystem::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
	getInstance().framebufferSizeCallbackImpl(window, width, height);
}

void GLInputSystem::mousePositionCallback(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	getInstance().mousePositionCallbackImpl(window, mouseXPos, mouseYPos);
}

void GLInputSystem::scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
	getInstance().scrollCallbackImpl(window, xoffset, yoffset);
}

void GLInputSystem::framebufferSizeCallbackImpl(GLFWwindow * window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void GLInputSystem::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	m_mouseXOffset = mouseXPos - m_mouseLastX;
	m_mouseYOffset = m_mouseLastY - mouseYPos;

	m_mouseLastX = mouseXPos;
	m_mouseLastY = mouseYPos;
}

void GLInputSystem::scrollCallbackImpl(GLFWwindow * window, double xoffset, double yoffset)
{
}

const objectStatus & GLInputSystem::getStatus() const
{
	return m_objectStatus;
}

void GLInputSystem::setup()
{
}

void GLInputSystem::initialize()
{
	glfwSetFramebufferSizeCallback(GLWindowSystem::getInstance().getWindow(), &framebufferSizeCallback);
	glfwSetCursorPosCallback(GLWindowSystem::getInstance().getWindow(), &mousePositionCallback);
	glfwSetScrollCallback(GLWindowSystem::getInstance().getWindow(), &scrollCallback);
	glfwSetInputMode(GLWindowSystem::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_TRUE);

	m_objectStatus = objectStatus::ALIVE;
	g_pLogSystem->printLog("GLInputSystem has been initialized.");
}

void GLInputSystem::update()
{
	if (GLWindowSystem::getInstance().getWindow() != nullptr)
	{

		for (int i = 0; i < NUM_KEYCODES; i++)
		{
			if (glfwGetKey(GLWindowSystem::getInstance().getWindow(), i) == GLFW_PRESS)
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


		if (glfwGetMouseButton(GLWindowSystem::getInstance().getWindow(), 1) == GLFW_PRESS)
		{
			GLWindowSystem::getInstance().hideMouseCursor();
			if (m_mouseMovementCallback.size() != 0)
			{
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
		}
		else
		{
			GLWindowSystem::getInstance().showMouseCursor();
		}
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("GLInputSystem is stand-by.");
	}
}

void GLInputSystem::shutdown()
{
	glfwSetInputMode(GLWindowSystem::getInstance().getWindow(), GLFW_STICKY_KEYS, GL_FALSE);
	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("GLInputSystem has been shutdown.");
}

