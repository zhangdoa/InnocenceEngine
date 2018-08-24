#pragma once
#include "BaseComponent.h"
#include "common/GLHeaders.h"
#include <windows.h>

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
	vec2 m_windowResolution = vec2(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

#if defined(INNO_RENDERER_DX)
	HINSTANCE m_hInstance;
	PSTR m_pScmdline;
	int m_nCmdshow;
	LPCSTR m_applicationName;
	HWND m_hwnd;
#endif

	//input data
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;

	buttonStatusMap m_buttonStatus;
	buttonStatusCallbackMap m_buttonStatusCallback;
	mouseMovementCallbackMap m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;

private:
	WindowSystemSingletonComponent() {};
};
