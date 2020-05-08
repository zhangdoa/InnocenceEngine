#include "WinDXWindowSurface.h"
#include "../WinWindowSystem.h"
#include "../../Engine/Core/InnoLogger.h"

#include "../../../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace WinDXWindowSurfaceNS
{
	bool setup(void* hInstance, void* hwnd, void* WindowProc);
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
}

bool WinDXWindowSurfaceNS::setup(void* hInstance, void* hwnd, void* WindowProc)
{
	m_initConfig = g_pModuleManager->getInitConfig();

	// Setup the windows class with default settings.
	auto l_windowName = g_pModuleManager->getApplicationName();

	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = (WNDPROC)WindowProc;
	wcex.hInstance = reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHInstance();
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getApplicationName();

	auto l_windowClass = MAKEINTATOM(RegisterClassEx(&wcex));

	// Determine the resolution of the clients desktop screen.
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_screenWidth = (int32_t)l_screenResolution.x;
	auto l_screenHeight = (int32_t)l_screenResolution.y;

	auto l_posX = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
	auto l_posY = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

	if (m_initConfig.engineMode == EngineMode::Host)
	{
		// create a new window and context
		auto l_hwnd = CreateWindow(
			l_windowClass, reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getApplicationName(), // class name, window name
			WS_OVERLAPPEDWINDOW, // styles
			l_posX, l_posY, // posx, posy. If x is set to CW_USEDEFAULT y is ignored
			l_screenWidth, l_screenHeight, // width, height
			NULL, NULL, // parent window, menu
			reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHInstance(), NULL); // instance, param

		reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->setHwnd(l_hwnd);
	}

	m_ObjectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "WinDXWindowSurface setup finished.");

	return true;
}

bool WinDXWindowSurfaceNS::initialize()
{
	if (m_initConfig.engineMode == EngineMode::Host)
	{
		// Bring the window up on the screen and set it as main focus.
		ShowWindow(reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHwnd(), true);
		SetForegroundWindow(reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHwnd());
		SetFocus(reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHwnd());
	}

	InnoLogger::Log(LogLevel::Success, "WinDXWindowSurface has been initialized.");
	return true;
}

bool WinDXWindowSurfaceNS::update()
{
	return true;
}

bool WinDXWindowSurfaceNS::terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "WinGLWindowSystemNS has been terminated.");

	return true;
}

bool WinDXWindowSurface::setup(void* hInstance, void* hwnd, void* WindowProc)
{
	return WinDXWindowSurfaceNS::setup(hInstance, hwnd, WindowProc);
}

bool WinDXWindowSurface::initialize()
{
	return WinDXWindowSurfaceNS::initialize();
}

bool WinDXWindowSurface::update()
{
	return WinDXWindowSurfaceNS::update();
}

bool WinDXWindowSurface::terminate()
{
	return WinDXWindowSurfaceNS::terminate();
}

ObjectStatus WinDXWindowSurface::getStatus()
{
	return WinDXWindowSurfaceNS::m_ObjectStatus;
}

bool WinDXWindowSurface::swapBuffer()
{
	return true;
}