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

																   // Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Test", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window); // Initialize GLEW
	glewExperimental = true; // Needed in core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	
	fprintf(stdout, "Window has been created.\n");
	isRunning = true;

	return 0;

}

void Window::update()
{
	if (isRunning)
	{
		if (window!=NULL && glfwWindowShouldClose(window) == 0) {
			glfwSwapBuffers(window);
			glfwPollEvents();
		}
		else { 			
			isRunning = false; 
			shutdown();
		}
	}
}

void Window::shutdown() { 
	if (window)
	{
		glfwDestroyWindow(window);
		window = NULL;
		fprintf(stdout, "Window has been destroyed.");
	}
}

GLFWwindow* Window::getWindow()
{
	return window;
}