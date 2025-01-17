#include "WinDXWindowSurface.h"
#include "../WinWindowSystem.h"
#include "../../../Common/LogService.h"
#include "../../../Services/RenderingConfigurationService.h"
#include "../../../Services/RenderingContextService.h"

#include "../../../Engine.h"
using namespace Inno;

bool WinDXWindowSurface::Setup(ISystemConfig* systemConfig)
{
	auto l_windowSurfaceConfig = reinterpret_cast<IWindowSurfaceConfig*>(systemConfig);

	auto l_InitConfig = g_Engine->getInitConfig();
	if (l_InitConfig.engineMode == EngineMode::Host)
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
		auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
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
		
		Log(Success, "A new window handle has been created.");
	}

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool WinDXWindowSurface::Initialize()
{
	auto l_InitConfig = g_Engine->getInitConfig();
	if (l_InitConfig.engineMode == EngineMode::Host)
	{
		// Bring the window up on the screen and set it as main focus.
		ShowWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle(), true);
		SetForegroundWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle());
		SetFocus(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle());

		Log(Success, "The window has been brought to the foreground.");
	}

	return true;
}

bool WinDXWindowSurface::Update()
{
	return true;
}

bool WinDXWindowSurface::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Warning, "The window needs to be terminated.");

	return true;
}

ObjectStatus WinDXWindowSurface::GetStatus()
{
	return m_ObjectStatus;
}

bool WinDXWindowSurface::swapBuffer()
{
	return true;
}