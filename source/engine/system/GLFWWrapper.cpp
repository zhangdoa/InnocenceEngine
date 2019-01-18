#include "GLFWWrapper.h"

namespace windowCallbackWrapperNS
{
	IInputSystem* m_IInputSystem;
}

void windowCallbackWrapper::initialize(GLFWwindow * window, IInputSystem* rhs)
{
	glfwSetFramebufferSizeCallback(window, &framebufferSizeCallback);
	glfwSetCursorPosCallback(window, &mousePositionCallback);
	glfwSetScrollCallback(window, &scrollCallback);
	windowCallbackWrapperNS::m_IInputSystem = rhs;
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
	windowCallbackWrapperNS::m_IInputSystem->framebufferSizeCallback(width, height);
}

void windowCallbackWrapper::mousePositionCallbackImpl(GLFWwindow * window, float mouseXPos, float mouseYPos)
{
	windowCallbackWrapperNS::m_IInputSystem->mousePositionCallback(mouseXPos, mouseYPos);
}

void windowCallbackWrapper::scrollCallbackImpl(GLFWwindow * window, float xoffset, float yoffset)
{
	windowCallbackWrapperNS::m_IInputSystem->scrollCallback(xoffset, yoffset);
}