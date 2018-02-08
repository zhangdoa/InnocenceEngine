#include "../../../main/stdafx.h"
#include "GLWindowManager.h"

GLFWwindow * GLWindowManager::getWindow() const
{
	return m_window;
}

vec2 GLWindowManager::getScreenCenterPosition() const
{
	return vec2(SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
}

vec2 GLWindowManager::getScreenResolution() const
{
	return vec2(SCR_WIDTH, SCR_HEIGHT);
}

void GLWindowManager::setWindowName(const std::string & windowName)
{
	m_windowName = windowName;
}

void GLWindowManager::hideMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GLWindowManager::showMouseCursor() const
{
	glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void GLWindowManager::setup()
{
}

void GLWindowManager::initialize()
{
	if (glfwInit() != GL_TRUE)
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("Failed to initialize GLFW.");
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
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.");
		glfwTerminate();
	}
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("Failed to initialize GLAD.");
	}
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("GLWindowManager has been initialized.");
}

void GLWindowManager::update()
{
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0) {
		glfwPollEvents();
		glfwSwapBuffers(m_window);
	}
	else
	{
		this->setStatus(objectStatus::STANDBY);
		LogManager::getInstance().printLog("GLWindowManager is stand-by.");
	}
}

void GLWindowManager::shutdown()
{
	if (m_window != nullptr)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		this->setStatus(objectStatus::SHUTDOWN);
		LogManager::getInstance().printLog("GLWindowManager has been shutdown.");
	}
}