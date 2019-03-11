#include "GLFWWrapper.h"

namespace windowCallbackWrapperNS
{
	IInputSystem* m_inputSystem;

	ButtonStatusMap m_buttonStatus;
}

bool windowCallbackWrapper::initialize(GLFWwindow * window, IInputSystem* rhs)
{
	glfwSetFramebufferSizeCallback(window, &framebufferSizeCallback);
	glfwSetCursorPosCallback(window, &mousePositionCallback);
	glfwSetScrollCallback(window, &scrollCallback);
	windowCallbackWrapperNS::m_inputSystem = rhs;

	for (int i = 0; i < rhs->getInputConfig().totalKeyCodes; i++)
	{
		windowCallbackWrapperNS::m_buttonStatus.emplace(i, ButtonStatus::RELEASED);
	}

	return true;
}

bool windowCallbackWrapper::update(GLFWwindow * window)
{	//keyboard
	for (int i = 0; i < windowCallbackWrapperNS::m_inputSystem->getInputConfig().totalKeyCodes; i++)
	{
		if (glfwGetKey(window, i) == GLFW_PRESS)
		{
			auto l_result = windowCallbackWrapperNS::m_buttonStatus.find(i);
			if (l_result != windowCallbackWrapperNS::m_buttonStatus.end())
			{
				l_result->second = ButtonStatus::PRESSED;
			}
		}
		else
		{
			auto l_result = windowCallbackWrapperNS::m_buttonStatus.find(i);
			if (l_result != windowCallbackWrapperNS::m_buttonStatus.end())
			{
				l_result->second = ButtonStatus::RELEASED;
			}
		}
	}
	//mouse
	for (int i = 0; i < windowCallbackWrapperNS::m_inputSystem->getInputConfig().totalMouseCodes; i++)
	{
		if (glfwGetMouseButton(window, i) == GLFW_PRESS)
		{
			auto l_result = windowCallbackWrapperNS::m_buttonStatus.find(i);
			if (l_result != windowCallbackWrapperNS::m_buttonStatus.end())
			{
				l_result->second = ButtonStatus::PRESSED;
			}
		}
		else
		{
			auto l_result = windowCallbackWrapperNS::m_buttonStatus.find(i);
			if (l_result != windowCallbackWrapperNS::m_buttonStatus.end())
			{
				l_result->second = ButtonStatus::RELEASED;
			}
		}
	}

	return true;
}

void windowCallbackWrapper::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
	get().framebufferSizeCallbackImpl(window, width, height);
}

void windowCallbackWrapper::mousePositionCallback(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	get().mousePositionCallbackImpl(window, (float)mouseXPos, (float)mouseYPos);
}

void windowCallbackWrapper::scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
	get().scrollCallbackImpl(window, (float)xoffset, (float)yoffset);
}

void windowCallbackWrapper::framebufferSizeCallbackImpl(GLFWwindow * window, int width, int height)
{
	windowCallbackWrapperNS::m_inputSystem->framebufferSizeCallback(width, height);
}

void windowCallbackWrapper::mousePositionCallbackImpl(GLFWwindow * window, float mouseXPos, float mouseYPos)
{
	windowCallbackWrapperNS::m_inputSystem->mousePositionCallback(mouseXPos, mouseYPos);
}

void windowCallbackWrapper::scrollCallbackImpl(GLFWwindow * window, float xoffset, float yoffset)
{
	windowCallbackWrapperNS::m_inputSystem->scrollCallback(xoffset, yoffset);
}

ButtonStatusMap windowCallbackWrapper::getButtonStatus()
{
	return windowCallbackWrapperNS::m_buttonStatus;
}
