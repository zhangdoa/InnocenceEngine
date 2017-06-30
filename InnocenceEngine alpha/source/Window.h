#pragma once
#include "stdafx.h"


class Window
{

public:
	Window();
	~Window();

	int init();
	void render();
	void shutdown();
	bool _isRunning;

	GLFWwindow* getWindow();

private:
	GLFWwindow* _window;

	

};

