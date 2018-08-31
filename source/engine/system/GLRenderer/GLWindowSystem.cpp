#include "GLWindowSystem.h"

void GLWindowSystem::setup()
{
	//setup window
	if (glfwInit() != GL_TRUE)
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to initialize GLFW.");
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
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.");
		glfwTerminate();
	}
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to initialize GLAD.");
	}

	//setup input
	glfwSetInputMode(WindowSystemSingletonComponent::getInstance().m_window, GLFW_STICKY_KEYS, GL_TRUE);

	BaseWindowSystem::setup();

	m_objectStatus = objectStatus::ALIVE;
}

void GLWindowSystem::initialize()
{
	//initialize window
	windowCallbackWrapper::getInstance().setWindowSystem(this);
	windowCallbackWrapper::getInstance().initialize();

	//initialize input

	BaseWindowSystem::initialize();

	g_pLogSystem->printLog("GLWindowSystem has been initialized.");
}

void GLWindowSystem::update()
{
	//update window
	if (WindowSystemSingletonComponent::getInstance().m_window == nullptr || glfwWindowShouldClose(WindowSystemSingletonComponent::getInstance().m_window) != 0)
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("GLWindowSystem: Input error or Window closed.");
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

	BaseWindowSystem::update();
}

void GLWindowSystem::shutdown()
{
	glfwSetInputMode(WindowSystemSingletonComponent::getInstance().m_window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwDestroyWindow(WindowSystemSingletonComponent::getInstance().m_window);
	glfwTerminate();
	g_pLogSystem->printLog("GLWindowSystem: Window closed.");

	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("GLWindowSystem has been shutdown.");
}

const objectStatus & GLWindowSystem::getStatus() const
{
	return m_objectStatus;
}

void GLWindowSystem::swapBuffer()
{
	glfwSwapBuffers(WindowSystemSingletonComponent::getInstance().m_window);
}

void GLWindowSystem::hideMouseCursor() const
{
	glfwSetInputMode(WindowSystemSingletonComponent::getInstance().m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GLWindowSystem::showMouseCursor() const
{
	glfwSetInputMode(WindowSystemSingletonComponent::getInstance().m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void windowCallbackWrapper::setWindowSystem(GLWindowSystem * GLWindowSystem)
{
	m_GLWindowSystem = GLWindowSystem;
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
	m_GLWindowSystem->framebufferSizeCallback(width, height);
}

void windowCallbackWrapper::mousePositionCallbackImpl(GLFWwindow * window, double mouseXPos, double mouseYPos)
{
	m_GLWindowSystem->mousePositionCallback(mouseXPos, mouseYPos);
}

void windowCallbackWrapper::scrollCallbackImpl(GLFWwindow * window, double xoffset, double yoffset)
{
	m_GLWindowSystem->scrollCallback(xoffset, yoffset);
}