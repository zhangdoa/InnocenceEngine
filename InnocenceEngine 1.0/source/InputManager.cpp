#include "stdafx.h"
#include "InputManager.h"


InputManager::InputManager()
{
}


InputManager::~InputManager()
{
}

int InputManager::getKey(int keyCode)
{
	return glfwGetKey(m_window, keyCode);
}

int InputManager::getMouse(int mouseButton)
{
	return glfwGetMouseButton(m_window, mouseButton);
}

void InputManager::setWindow(GLFWwindow * window)
{
	m_window = window;
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_TRUE);
}

void InputManager::init()
{
	std::vector<int>lastKeys(NUM_KEYCODES);
	std::vector<int>lastMouse(NUM_MOUSEBUTTONS);

	this->setStatus(INITIALIZIED);
	LogManager::printLog("InputManager has been initialized.");
}

void InputManager::update()
{
	if (m_window != nullptr) 
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
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_FALSE);
	this->setStatus(UNINITIALIZIED);
	LogManager::printLog("InputManager has been shutdown.");
}
