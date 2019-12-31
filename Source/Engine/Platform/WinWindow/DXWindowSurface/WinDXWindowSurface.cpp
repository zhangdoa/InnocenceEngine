#include "WinDXWindowSurface.h"
#include "../../../Component/WinWindowSystemComponent.h"
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
	WNDCLASS wc = {};

	auto l_windowName = g_pModuleManager->getApplicationName();

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = WinWindowSystemComponent::get().m_hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = WinWindowSystemComponent::get().m_applicationName;

	// Register the window class.
	RegisterClass(&wc);

	// Determine the resolution of the clients desktop screen.
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_screenWidth = (int32_t)l_screenResolution.x;
	auto l_screenHeight = (int32_t)l_screenResolution.y;

	auto l_posX = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
	auto l_posY = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

	if (m_initConfig.engineMode == EngineMode::Host)
	{
		// Create the window with the screen settings and get the handle to it.
		WinWindowSystemComponent::get().m_hwnd = CreateWindowEx(0, WinWindowSystemComponent::get().m_applicationName, (LPCSTR)l_windowName.c_str(),
			WS_OVERLAPPEDWINDOW,
			l_posX, l_posY, l_screenWidth, l_screenHeight, NULL, NULL, WinWindowSystemComponent::get().m_hInstance, NULL);
	}

	WinWindowSystemComponent::get().m_HDC = GetDC(WinWindowSystemComponent::get().m_hwnd);

	if (m_initConfig.engineMode == EngineMode::Host)
	{
		// Bring the window up on the screen and set it as main focus.
		ShowWindow(WinWindowSystemComponent::get().m_hwnd, true);
		SetForegroundWindow(WinWindowSystemComponent::get().m_hwnd);
		SetFocus(WinWindowSystemComponent::get().m_hwnd);
	}

	m_ObjectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "WinDXWindowSurface setup finished.");

	return true;
}

bool WinDXWindowSurfaceNS::initialize()
{
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