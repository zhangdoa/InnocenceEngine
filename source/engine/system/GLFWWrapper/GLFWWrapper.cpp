#include "GLFWWrapper.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLFWWrapperNS
{
	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, float mouseXPos, float mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, float xoffset, float yoffset);


	InputConfig m_InputConfig;
	ButtonStatusMap m_buttonStatus;
}

bool GLFWWrapper::setup()
{
	GLFWWrapperNS::m_InputConfig = g_pCoreSystem->getInputSystem()->getInputConfig();
	auto l_initConfig = g_pCoreSystem->getVisionSystem()->getInitConfig();

	//setup window
	if (glfwInit() != GL_TRUE)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLFWWrapper: Failed to initialize GLFW.");
		return false;
	}

	glfwSetFramebufferSizeCallback(m_window, &framebufferSizeCallback);
	glfwSetCursorPosCallback(m_window, &mousePositionCallback);
	glfwSetScrollCallback(m_window, &scrollCallback);

	if (l_initConfig.renderingBackend == RenderingBackend::GL)
	{
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
#ifdef INNO_PLATFORM_MAC
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
#endif
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL
	}
	else if (l_initConfig.renderingBackend == RenderingBackend::VK)
	{
		// bind Vulkan API hook later
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLFWWrapper: Unsupported rendering backend!");
		return false;
	}

	// Open a window and create its OpenGL context
	auto l_screenResolution = g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->getScreenResolution();
	GLFWWrapper::get().m_window = glfwCreateWindow((int)l_screenResolution.x, (int)l_screenResolution.y, g_pCoreSystem->getGameSystem()->getGameName().c_str(), NULL, NULL);
	glfwMakeContextCurrent(GLFWWrapper::get().m_window);

	if (GLFWWrapper::get().m_window == nullptr)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLFWWrapper: Failed to open GLFW window.");
		glfwTerminate();
		return false;
	}

	//setup input
	glfwSetInputMode(GLFWWrapper::get().m_window, GLFW_STICKY_KEYS, GL_TRUE);

	for (int i = 0; i < GLFWWrapperNS::m_InputConfig.totalKeyCodes; i++)
	{
		GLFWWrapperNS::m_buttonStatus.emplace(i, ButtonStatus::RELEASED);
	}

	return true;
}

bool GLFWWrapper::initialize()
{
	return true;
}

bool GLFWWrapper::update()
{
	//update window
	if (m_window == nullptr || glfwWindowShouldClose(m_window) != 0)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VKWindowSystem: Input error or Window closed.");
		return false;
	}
	else
	{
		glfwPollEvents();

		//keyboard
		for (int i = 0; i < GLFWWrapperNS::m_InputConfig.totalKeyCodes; i++)
		{
			if (glfwGetKey(m_window, i) == GLFW_PRESS)
			{
				auto l_result = GLFWWrapperNS::m_buttonStatus.find(i);
				if (l_result != GLFWWrapperNS::m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = GLFWWrapperNS::m_buttonStatus.find(i);
				if (l_result != GLFWWrapperNS::m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::RELEASED;
				}
			}
		}
		//mouse
		for (int i = 0; i < GLFWWrapperNS::m_InputConfig.totalMouseCodes; i++)
		{
			if (glfwGetMouseButton(m_window, i) == GLFW_PRESS)
			{
				auto l_result = GLFWWrapperNS::m_buttonStatus.find(i);
				if (l_result != GLFWWrapperNS::m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = GLFWWrapperNS::m_buttonStatus.find(i);
				if (l_result != GLFWWrapperNS::m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::RELEASED;
				}
			}
		}

		return true;
	}
}

bool GLFWWrapper::terminate()
{
	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwDestroyWindow(m_window);
	glfwTerminate();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLFWWrapper has been terminated.");
	return true;
}

void GLFWWrapper::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
	GLFWWrapperNS::framebufferSizeCallbackImpl(window, width, height);
}

void GLFWWrapper::mousePositionCallback(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	GLFWWrapperNS::mousePositionCallbackImpl(window, (float)mouseXPos, (float)mouseYPos);
}

void GLFWWrapper::scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
	GLFWWrapperNS::scrollCallbackImpl(window, (float)xoffset, (float)yoffset);
}

void GLFWWrapperNS::framebufferSizeCallbackImpl(GLFWwindow * window, int width, int height)
{
	g_pCoreSystem->getInputSystem()->framebufferSizeCallback(width, height);
}

void GLFWWrapperNS::mousePositionCallbackImpl(GLFWwindow * window, float mouseXPos, float mouseYPos)
{
	g_pCoreSystem->getInputSystem()->mousePositionCallback(mouseXPos, mouseYPos);
}

void GLFWWrapperNS::scrollCallbackImpl(GLFWwindow * window, float xoffset, float yoffset)
{
	g_pCoreSystem->getInputSystem()->scrollCallback(xoffset, yoffset);
}

ButtonStatusMap GLFWWrapper::getButtonStatus()
{
	return GLFWWrapperNS::m_buttonStatus;
}

void GLFWWrapper::swapBuffer()
{
	glfwSwapBuffers(m_window);
}

void GLFWWrapper::hideMouseCursor()
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GLFWWrapper::showMouseCursor()
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}