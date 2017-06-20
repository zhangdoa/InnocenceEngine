#pragma once
#include "stdafx.h"
#include "Window.h"
class Input
{
public:
	Input();
	~Input();
	void init();
	void update();
	void shutdown();
	int getKey(int keyCode);
	int getMouse(int mouseButton);
	void setWindow(Window* currentWindow);

private:
	Window* window;
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;
	std::vector<int>lastKeys;
	std::vector<int>lastMouse;
};

