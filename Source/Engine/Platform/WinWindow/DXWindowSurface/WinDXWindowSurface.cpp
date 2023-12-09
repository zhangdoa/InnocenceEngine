#include "WinDXWindowSurface.h"
#include "../WinWindowSystem.h"
#include "../../../Core/Logger.h"

#include "../../../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace WinDXWindowSurfaceNS
{
	bool Setup(ISystemConfig* systemConfig);
	bool Initialize();
	bool Update();
	bool Terminate();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_InitConfig;
}

bool WinDXWindowSurfaceNS::Setup(ISystemConfig* systemConfig)
{
	auto l_windowSurfaceConfig = reinterpret_cast<IWindowSurfaceConfig*>(systemConfig);

	m_InitConfig = g_Engine->getInitConfig();

	if (m_InitConfig.engineMode == EngineMode::Host)
	{
		// Setup the windows class with default settings.
		auto l_windowName = g_Engine->GetApplicationName();

		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wcex.lpfnWndProc = (WNDPROC)l_windowSurfaceConfig->WindowProc;
		wcex.hInstance = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetApplicationInstance();
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpszClassName = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetApplicationName();

		auto l_windowClass = MAKEINTATOM(RegisterClassEx(&wcex));

		// Determine the resolution of the clients desktop screen.
		auto l_screenResolution = g_Engine->getRenderingFrontend()->GetScreenResolution();
		auto l_screenWidth = (int32_t)l_screenResolution.x;
		auto l_screenHeight = (int32_t)l_screenResolution.y;

		RECT l_rect;
		l_rect.right = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
		l_rect.bottom = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

		AdjustWindowRect(&l_rect, WS_OVERLAPPEDWINDOW, false);

		// create a new window and context
		auto l_hwnd = CreateWindow(
			l_windowClass, reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetApplicationName(), // class name, window name
			WS_OVERLAPPEDWINDOW, // styles
			l_rect.right, l_rect.bottom, // posx, posy. If x is set to CW_USEDEFAULT y is ignored
			l_screenWidth, l_screenHeight, // width, height
			NULL, NULL, // parent window, menu
			reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetApplicationInstance(), NULL); // instance, param

		reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->SetWindowHandle(l_hwnd);
	}

	m_ObjectStatus = ObjectStatus::Activated;
	Logger::Log(LogLevel::Success, "WinDXWindowSurface Setup finished.");

	return true;
}

bool WinDXWindowSurfaceNS::Initialize()
{
	if (m_InitConfig.engineMode == EngineMode::Host)
	{
		// Bring the window up on the screen and set it as main focus.
		ShowWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle(), true);
		SetForegroundWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle());
		SetFocus(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle());
	}

	Logger::Log(LogLevel::Success, "WinDXWindowSurface has been initialized.");
	return true;
}

bool WinDXWindowSurfaceNS::Update()
{
	return true;
}

bool WinDXWindowSurfaceNS::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Logger::Log(LogLevel::Success, "WinGLWindowSystemNS has been terminated.");

	return true;
}

bool WinDXWindowSurface::Setup(ISystemConfig* systemConfig)
{
	return WinDXWindowSurfaceNS::Setup(systemConfig);
}

bool WinDXWindowSurface::Initialize()
{
	return WinDXWindowSurfaceNS::Initialize();
}

bool WinDXWindowSurface::Update()
{
	return WinDXWindowSurfaceNS::Update();
}

bool WinDXWindowSurface::Terminate()
{
	return WinDXWindowSurfaceNS::Terminate();
}

ObjectStatus WinDXWindowSurface::GetStatus()
{
	return WinDXWindowSurfaceNS::m_ObjectStatus;
}

bool WinDXWindowSurface::swapBuffer()
{
	return true;
}