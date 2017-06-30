#include "Window.h"


Window::Window()
{
	init();
}


Window::~Window()
{
	shutdown();
}

int Window::init()
{
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 

																   // Open a _window and create its OpenGL context
	_window = glfwCreateWindow(1024, 768, "Test", NULL, NULL);
	if (_window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(_window); // Initialize GLEW
	glewExperimental = true; // Needed in core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	
	fprintf(stdout, "Window has been created.\n");
	_isRunning = true;

	return 0;

}

void Window::render()
{
	//fprintf(stdout, "Window is rendering.\n");
	if (_isRunning)
	{
		if (_window != nullptr && glfwWindowShouldClose(_window) == 0) {
			glfwSwapBuffers(_window);
			glfwPollEvents();
		}
		else { 			
			_isRunning = false; 
			shutdown();
		}
	}
}

void Window::shutdown() { 
	if (_window)
	{
		glfwDestroyWindow(_window);
		delete _window;
		fprintf(stdout, "Window has been destroyed.");
	}
}

GLFWwindow* Window::getWindow()
{
	return _window;
}