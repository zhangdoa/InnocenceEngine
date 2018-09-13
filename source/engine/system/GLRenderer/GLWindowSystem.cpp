#include "GLWindowSystem.h"
#include "../LogSystem.h"
#include "../InputSystem.h"

class windowCallbackWrapper
{
public:
	~windowCallbackWrapper() {};

	static windowCallbackWrapper& getInstance()
	{
		static windowCallbackWrapper instance;
		return instance;
	}
	void initialize();

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);

private:
	windowCallbackWrapper() {};
};

namespace GLWindowSystem
{
	objectStatus m_WindowSystemStatus = objectStatus::SHUTDOWN;
}

void GLWindowSystem::setup()
{
	//setup window
	if (glfwInit() != GL_TRUE)
	{
		m_WindowSystemStatus = objectStatus::STANDBY;
		InnoLogSystem::printLog("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#ifdef INNO_PLATFORM_MACOS
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL

	// Open a window and create its OpenGL context
	WindowSystemSingletonComponent::getInstance().m_window = glfwCreateWindow((int)WindowSystemSingletonComponent::getInstance().m_windowResolution.x, (int)WindowSystemSingletonComponent::getInstance().m_windowResolution.y, WindowSystemSingletonComponent::getInstance().m_windowName.c_str(), NULL, NULL);
	glfwMakeContextCurrent(WindowSystemSingletonComponent::getInstance().m_window);
	if (WindowSystemSingletonComponent::getInstance().m_window == nullptr) {
		m_WindowSystemStatus = objectStatus::STANDBY;
		InnoLogSystem::printLog("Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.");
		glfwTerminate();
	}
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		m_WindowSystemStatus = objectStatus::STANDBY;
		InnoLogSystem::printLog("Failed to initialize GLAD.");
	}

	//setup input
	glfwSetInputMode(WindowSystemSingletonComponent::getInstance().m_window, GLFW_STICKY_KEYS, GL_TRUE);

	InnoInputSystem::setup();

	m_WindowSystemStatus = objectStatus::ALIVE;
}

void GLWindowSystem::initialize()
{
	//initialize window
	windowCallbackWrapper::getInstance().initialize();

	//initialize input
	
	InnoInputSystem::initialize();

	InnoLogSystem::printLog("GLWindowSystem has been initialized.");
}

void GLWindowSystem::update()
{
	//update window
	if (WindowSystemSingletonComponent::getInstance().m_window == nullptr || glfwWindowShouldClose(WindowSystemSingletonComponent::getInstance().m_window) != 0)
	{
		m_WindowSystemStatus = objectStatus::STANDBY;
		InnoLogSystem::printLog("GLWindowSystem: Input error or Window closed.");
	}
	else
	{
		glfwPollEvents();

		//update input
		//keyboard
		for (int i = 0; i < WindowSystemSingletonComponent::getInstance().NUM_KEYCODES; i++)
		{
			if (glfwGetKey(WindowSystemSingletonComponent::getInstance().m_window, i) == GLFW_PRESS)
			{
				auto l_result = WindowSystemSingletonComponent::getInstance().m_buttonStatus.find(i);
				if (l_result != WindowSystemSingletonComponent::getInstance().m_buttonStatus.end())
				{
					l_result->second = buttonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = WindowSystemSingletonComponent::getInstance().m_buttonStatus.find(i);
				if (l_result != WindowSystemSingletonComponent::getInstance().m_buttonStatus.end())
				{
					l_result->second = buttonStatus::RELEASED;
				}
			}
		}
		//mouse
		for (int i = 0; i < WindowSystemSingletonComponent::getInstance().NUM_MOUSEBUTTONS; i++)
		{
			if (glfwGetMouseButton(WindowSystemSingletonComponent::getInstance().m_window, i) == GLFW_PRESS)
			{
				auto l_result = WindowSystemSingletonComponent::getInstance().m_buttonStatus.find(i);
				if (l_result != WindowSystemSingletonComponent::getInstance().m_buttonStatus.end())
				{
					l_result->second = buttonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = WindowSystemSingletonComponent::getInstance().m_buttonStatus.find(i);
				if (l_result != WindowSystemSingletonComponent::getInstance().m_buttonStatus.end())
				{
					l_result->second = buttonStatus::RELEASED;
				}
			}
		}
	}

	InnoInputSystem::update();
}

void GLWindowSystem::shutdown()
{
	glfwSetInputMode(WindowSystemSingletonComponent::getInstance().m_window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwDestroyWindow(WindowSystemSingletonComponent::getInstance().m_window);
	glfwTerminate();
	InnoLogSystem::printLog("GLWindowSystem: Window closed.");

	m_WindowSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("GLWindowSystem has been shutdown.");
}

void GLWindowSystem::swapBuffer()
{
	glfwSwapBuffers(WindowSystemSingletonComponent::getInstance().m_window);
}

void GLWindowSystem::hideMouseCursor()
{
	glfwSetInputMode(WindowSystemSingletonComponent::getInstance().m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GLWindowSystem::showMouseCursor()
{
	glfwSetInputMode(WindowSystemSingletonComponent::getInstance().m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

objectStatus GLWindowSystem::getStatus()
{
	return m_WindowSystemStatus;
}

void windowCallbackWrapper::initialize()
{
	glfwSetFramebufferSizeCallback(WindowSystemSingletonComponent::getInstance().m_window, &framebufferSizeCallback);
	glfwSetCursorPosCallback(WindowSystemSingletonComponent::getInstance().m_window, &mousePositionCallback);
	glfwSetScrollCallback(WindowSystemSingletonComponent::getInstance().m_window, &scrollCallback);
}

void windowCallbackWrapper::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
	getInstance().framebufferSizeCallbackImpl(window, width, height);
}

void windowCallbackWrapper::mousePositionCallback(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	getInstance().mousePositionCallbackImpl(window, mouseXPos, mouseYPos);
}

void windowCallbackWrapper::scrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
	getInstance().scrollCallbackImpl(window, xoffset, yoffset);
}

void windowCallbackWrapper::framebufferSizeCallbackImpl(GLFWwindow * window, int width, int height)
{
	InnoInputSystem::framebufferSizeCallback(width, height);
}

void windowCallbackWrapper::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	InnoInputSystem::mousePositionCallback(mouseXPos, mouseYPos);
}

void windowCallbackWrapper::scrollCallbackImpl(GLFWwindow * window, double xoffset, double yoffset)
{
	InnoInputSystem::scrollCallback(xoffset, yoffset);
}