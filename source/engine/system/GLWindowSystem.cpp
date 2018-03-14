#include "GLWindowSystem.h"

GLFWwindow * GLWindowSystem::getWindow() const
{
	return m_window;
}

vec2 GLWindowSystem::getScreenCenterPosition() const
{
	return vec2(SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
}

vec2 GLWindowSystem::getScreenResolution() const
{
	return vec2(SCR_WIDTH, SCR_HEIGHT);
}

void GLWindowSystem::setWindowName(const std::string & windowName)
{
	m_windowName = windowName;
}

void GLWindowSystem::hideMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GLWindowSystem::showMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

const objectStatus & GLWindowSystem::getStatus() const
{
	return m_objectStatus;
}

void GLWindowSystem::setup()
{
}

void GLWindowSystem::initialize()
{
	if (glfwInit() != GL_TRUE)
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 

	// Open a window and create its OpenGL context
	m_window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, m_windowName.c_str(), NULL, NULL);
	glfwMakeContextCurrent(m_window);
	if (m_window == nullptr) {
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
	m_objectStatus = objectStatus::ALIVE;
	g_pLogSystem->printLog("GLWindowSystem has been initialized.");
}

void GLWindowSystem::update()
{
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0) {
		glfwPollEvents();
		glfwSwapBuffers(m_window);
	}
	else
	{
		m_objectStatus = objectStatus::STANDBY;
		g_pLogSystem->printLog("GLWindowSystem is stand-by.");
	}
}

void GLWindowSystem::shutdown()
{
	if (m_window != nullptr)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		m_objectStatus = objectStatus::SHUTDOWN;
		g_pLogSystem->printLog("GLWindowSystem has been shutdown.");
	}
}