#include "WinWindowService.h"

#include "../../Common/LogService.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/ConfigurationService.h"

#include "DXWindowSurface/WinDXWindowSurface.h"
#include "VKWindowSurface/WinVKWindowSurface.h"

#include "../../Engine.h"

using namespace Inno;

bool WinWindowService::Setup(IServiceConfig* systemConfig)
{
	auto l_systemConfig = reinterpret_cast<IWindowServiceConfig*>(systemConfig);
	m_ApplicationInstance = static_cast<HINSTANCE>(l_systemConfig->m_AppHook);
	if (l_systemConfig->m_ExtraHook)
	{
		m_WindowHandle = *reinterpret_cast<HWND*>(l_systemConfig->m_ExtraHook);
	}

	m_ApplicationName = g_Engine->GetApplicationName().c_str();

	Log(Success, "WinWindowService::Setup: engineMode=", (int)g_Engine->Get<ConfigurationService>()->GetEngineMode());

	switch (g_Engine->Get<ConfigurationService>()->GetGraphicsService())
{
	case GraphicsService::DX12:
#if defined INNO_PLATFORM_WIN
		m_WindowSurface = new WinDXWindowSurface();
#endif
		break;
	case GraphicsService::VK:
#if defined INNO_RENDERER_VULKAN
		m_WindowSurface = new WinVKWindowSurface();
#endif
		break;
	default:
		break;
	}

	if (g_Engine->Get<ConfigurationService>()->GetEngineMode() == EngineMode::Host || g_Engine->Get<ConfigurationService>()->GetEngineMode() == EngineMode::Sidecar)
	{
		auto l_windowName = g_Engine->GetApplicationName();

		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wcex.lpfnWndProc = (WNDPROC)WinWindowService::WindowProcedure;
		wcex.hInstance = reinterpret_cast<WinWindowService*>(g_Engine->getWindowService())->GetApplicationInstance();
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpszClassName = reinterpret_cast<WinWindowService*>(g_Engine->getWindowService())->GetApplicationName();

		auto l_windowClass = MAKEINTATOM(RegisterClassEx(&wcex));

		auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
		auto l_screenWidth = (int32_t)l_screenResolution.x;
		auto l_screenHeight = (int32_t)l_screenResolution.y;

		RECT l_rect = {0, 0, l_screenWidth, l_screenHeight};

		AdjustWindowRect(&l_rect, WS_OVERLAPPEDWINDOW, false);
		int actualWindowWidth  = l_rect.right - l_rect.left;
		int actualWindowHeight = l_rect.bottom - l_rect.top;

		int screenW = GetSystemMetrics(SM_CXSCREEN);
		int screenH = GetSystemMetrics(SM_CYSCREEN);
		int posX = (screenW - actualWindowWidth) / 2;
		int posY = (screenH - actualWindowHeight) / 2;

		auto l_hwnd = CreateWindow(
			l_windowClass, reinterpret_cast<WinWindowService*>(g_Engine->getWindowService())->GetApplicationName(),
			WS_OVERLAPPEDWINDOW,
			posX, posY,
			actualWindowWidth, actualWindowHeight,
			NULL, NULL,
			m_ApplicationInstance, NULL);

		m_WindowHandle = l_hwnd;

		Log(Success, "A new window handle has been created.");
	}

	m_WindowSurface->Setup();

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "WinWindowService Setup finished.");

	return true;
}

bool WinWindowService::Initialize()
{
	m_WindowSurface->Initialize();

	if (g_Engine->Get<ConfigurationService>()->GetEngineMode() == EngineMode::Host || g_Engine->Get<ConfigurationService>()->GetEngineMode() == EngineMode::Sidecar)
	{
		ShowWindow(reinterpret_cast<WinWindowService*>(g_Engine->getWindowService())->GetWindowHandle(), true);
		SetForegroundWindow(reinterpret_cast<WinWindowService*>(g_Engine->getWindowService())->GetWindowHandle());
		SetFocus(reinterpret_cast<WinWindowService*>(g_Engine->getWindowService())->GetWindowHandle());

		Log(Success, "The window has been brought to the foreground.");
	}

	Log(Success, "WinWindowService has been initialized.");
	return true;
}

bool WinWindowService::Update()
{
	if (g_Engine->Get<ConfigurationService>()->GetEngineMode() != EngineMode::Host && g_Engine->Get<ConfigurationService>()->GetEngineMode() != EngineMode::Sidecar)
		return true;

	MSG msg = { 0 };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

bool WinWindowService::Terminate()
{
	m_WindowSurface->Terminate();

	if (g_Engine->Get<ConfigurationService>()->GetEngineMode() == EngineMode::Host || g_Engine->Get<ConfigurationService>()->GetEngineMode() == EngineMode::Sidecar)
	{
		ShowCursor(true);

		DestroyWindow(m_WindowHandle);
		m_WindowHandle = NULL;

		Log(Warning, "Window closed.");

		UnregisterClass(m_ApplicationName, m_ApplicationInstance);
		m_ApplicationInstance = NULL;
	}

	PostQuitMessage(0);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "WinWindowService has been terminated.");
	return true;
}

ObjectStatus WinWindowService::GetStatus()
{
	return m_ObjectStatus;
}

IWindowSurface* WinWindowService::GetWindowSurface()
{
	return m_WindowSurface;
}

LPCSTR WinWindowService::GetApplicationName()
{
	return m_ApplicationName;
}

HINSTANCE WinWindowService::GetApplicationInstance()
{
	return m_ApplicationInstance;
}

HWND WinWindowService::GetWindowHandle()
{
	return m_WindowHandle;
}

bool WinWindowService::SetWindowHandle(HWND hwnd)
{
	m_WindowHandle = hwnd;
	return true;
}
