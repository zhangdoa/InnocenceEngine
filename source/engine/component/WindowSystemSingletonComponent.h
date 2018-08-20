#pragma once
#include "BaseComponent.h"
#include "common/GLHeaders.h"

class WindowSystemSingletonComponent : public BaseComponent
{
public:
	~WindowSystemSingletonComponent() {};
	
	static WindowSystemSingletonComponent& getInstance()
	{
		static WindowSystemSingletonComponent instance;
		return instance;
	}

	//window data
	GLFWwindow* m_window;
	vec2 m_screenResolution = vec2(1280, 720);
	std::string m_windowName;

	//input data
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;

	std::unordered_map<int, keyButton> m_keyButtonMap;
	std::unordered_map<int, std::vector<std::function<void()>*>> m_keyboardInputCallback;
	std::unordered_map<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;

private:
	WindowSystemSingletonComponent() {};
};
