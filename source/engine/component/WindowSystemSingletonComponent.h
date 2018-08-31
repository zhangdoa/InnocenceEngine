#pragma once
#include "BaseComponent.h"
#if defined (INNO_RENDERER_OPENGL)
#include "../system/GLRenderer/GLHeaders.h"
#elif defined (INNO_RENDERER_DX)
#include <windows.h>
#endif

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
	vec2<double> m_windowResolution = vec2<double>(1280, 720);
	std::string m_windowName;
	bool m_fullScreen = false;

#if defined (INNO_RENDERER_OPENGL)
	GLFWwindow* m_window;
#elif defined (INNO_RENDERER_DX)
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
