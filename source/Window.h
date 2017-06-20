#pragma once
#include "stdafx.h"


class Window
{

public:
	Window();
	~Window();

	int init();
	void update();
	void shutdown();
	bool isRunning;

	GLFWwindow* getWindow();

private:
	GLFWwindow* window;

	

};

