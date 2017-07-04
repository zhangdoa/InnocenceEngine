#include "stdafx.h"
#include "UIManager.h"


UIManager::UIManager()
{
}


UIManager::~UIManager()
{
}

GLFWwindow * UIManager::getWindow()
{
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0) {
		return m_window;
	}
	else
	{
		return nullptr;
	}
}

void UIManager::init()
{
	if (glfwInit() != GL_TRUE)
	{
		this->setStatus(ERROR);
		printLog("Failed to initialize GLFW.");
	}

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 

																   // Open a window and create its OpenGL context
	m_window = glfwCreateWindow(1024, 768, "Test", NULL, NULL);
	if (m_window == nullptr) {
		this->setStatus(ERROR);
		printLog("Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.");
		glfwTerminate();
	}
	glfwMakeContextCurrent(m_window); // Initialize GLEW
	glewExperimental = true; // Needed in core profile
	if (glewInit() != GLEW_OK) {
		this->setStatus(ERROR);
		printLog("Failed to initialize GLEW.");
	}
	this->setStatus(INITIALIZIED);
	printLog("UIManager has been initialized.");
}

void UIManager::update()
{
	if (m_window != nullptr && glfwWindowShouldClose(m_window) == 0) {
		glfwSwapBuffers(m_window);
		glfwPollEvents();
	}
	else
	{
		this->setStatus(STANDBY);
		printLog("UIManager is stand-by.");
	}
}

void UIManager::shutdown()
{
	if (m_window != nullptr)
	{
		glfwDestroyWindow(m_window);
		glfwTerminate();
		this->setStatus(UNINITIALIZIED);
		printLog("UIManager has been shutdown.");
	}
}