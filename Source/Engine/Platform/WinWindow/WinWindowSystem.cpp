#include "WinWindowSystem.h"

#include "../../Common/LogService.h"
#include "../../Services/HIDService.h"
#include "../../Services/RenderingConfigurationService.h"
#include "../../Services/RenderingContextService.h"

#include "DXWindowSurface/WinDXWindowSurface.h"
#include "VKWindowSurface/WinVKWindowSurface.h"

#include "../../Engine.h"

using namespace Inno;

bool WinWindowSystem::Setup(ISystemConfig* systemConfig)
{
	auto l_systemConfig = reinterpret_cast<IWindowSystemConfig*>(systemConfig);
	m_ApplicationInstance = static_cast<HINSTANCE>(l_systemConfig->m_AppHook);
	if (l_systemConfig->m_ExtraHook)
	{
		m_WindowHandle = *reinterpret_cast<HWND*>(l_systemConfig->m_ExtraHook);
	}

	m_ApplicationName = g_Engine->GetApplicationName().c_str();
	m_InitConfig = g_Engine->getInitConfig();
	switch (m_InitConfig.renderingServer)
	{
	case RenderingServer::DX12:
#if defined INNO_PLATFORM_WIN
		m_WindowSurface = new WinDXWindowSurface();
#endif
		break;
	case RenderingServer::VK:
#if defined INNO_RENDERER_VULKAN
		m_WindowSurface = new WinVKWindowSurface();
#endif
		break;
	default:
		break;
	}

	if (m_InitConfig.engineMode == EngineMode::Host)
	{
		// Setup the windows class with default settings.
		auto l_windowName = g_Engine->GetApplicationName();

		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wcex.lpfnWndProc = (WNDPROC)WinWindowSystem::WindowProcedure;
		wcex.hInstance = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetApplicationInstance();
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpszClassName = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetApplicationName();

		auto l_windowClass = MAKEINTATOM(RegisterClassEx(&wcex));

		// Determine the resolution of the clients desktop screen.
		auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
		auto l_screenWidth = (int32_t)l_screenResolution.x;
		auto l_screenHeight = (int32_t)l_screenResolution.y;

		RECT l_rect = {0, 0, l_screenWidth, l_screenHeight};

		AdjustWindowRect(&l_rect, WS_OVERLAPPEDWINDOW, false);
		int actualWindowWidth  = l_rect.right - l_rect.left;
		int actualWindowHeight = l_rect.bottom - l_rect.top;

		// Center the window on screen:
		int screenW = GetSystemMetrics(SM_CXSCREEN);
		int screenH = GetSystemMetrics(SM_CYSCREEN);
		int posX = (screenW - actualWindowWidth) / 2;
		int posY = (screenH - actualWindowHeight) / 2;
		
		// create a new window and context
		auto l_hwnd = CreateWindow(
			l_windowClass, reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetApplicationName(), // class name, window name
			WS_OVERLAPPEDWINDOW, // styles
			posX, posY, // posx, posy. If x is set to CW_USEDEFAULT y is ignored
			actualWindowWidth, actualWindowHeight, // width, height
			NULL, NULL, // parent window, menu
			m_ApplicationInstance, NULL); // instance, param

		m_WindowHandle = l_hwnd;

		Log(Success, "A new window handle has been created.");
	}

	m_WindowSurface->Setup();

	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "WinWindowSystem Setup finished.");

	return true;
}

bool WinWindowSystem::Initialize()
{
	m_WindowSurface->Initialize();

	if (m_InitConfig.engineMode == EngineMode::Host)
	{
		// Bring the window up on the screen and set it as main focus.
		ShowWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle(), true);
		SetForegroundWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle());
		SetFocus(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->GetWindowHandle());

		Log(Success, "The window has been brought to the foreground.");
	}

	Log(Success, "WinWindowSystem has been initialized.");
	return true;
}

bool WinWindowSystem::Update()
{
	if (m_InitConfig.engineMode != EngineMode::Host)
		return true;

	MSG msg = { 0 };
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return true;
}

bool WinWindowSystem::Terminate()
{
	m_WindowSurface->Terminate();

	if (m_InitConfig.engineMode == EngineMode::Host)
	{
		// Show the mouse cursor.
		ShowCursor(true);

		// Remove the window.
		DestroyWindow(m_WindowHandle);
		m_WindowHandle = NULL;

		Log(Warning, "Window closed.");

		// Remove the application instance.
		UnregisterClass(m_ApplicationName, m_ApplicationInstance);
		m_ApplicationInstance = NULL;
	}

	PostQuitMessage(0);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "WinWindowSystem has been terminated.");
	return true;
}

ObjectStatus WinWindowSystem::GetStatus()
{
	return m_ObjectStatus;
}

IWindowSurface* WinWindowSystem::GetWindowSurface()
{
	return m_WindowSurface;
}

void WinWindowSystem::ConsumeEvents(const WindowEventProcessCallback& p_Callback)
{
	m_WindowEvents.Read([&](auto const& l_FrontBuffer)
		{
			p_Callback(l_FrontBuffer);
		});

	m_WindowEvents.Flip();
	m_WindowEvents.Write([](auto& l_BackBuffer)
		{
			for (auto i : l_BackBuffer)
			{
				delete i;
			}
			l_BackBuffer.clear();
		});
}

bool WinWindowSystem::SendEvent(void* windowHook, uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
	for (auto i : m_WindowEventCallbacks)
	{
		(*i)(windowHook, uMsg, (uint64_t)wParam, (int64_t)lParam);
	}

	HWND hwnd = (HWND)windowHook;
	switch (uMsg)
	{
	case WM_DESTROY:
	{
		Log(Warning, "WM_DESTROY signal received.");
		m_ObjectStatus = ObjectStatus::Suspended;
		return true;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
		return true;
	}
	case WM_SIZE:
	{
		if (lParam && g_Engine->GetStatus() == ObjectStatus::Activated)
		{
			auto l_width = lParam & 0xffff;
			auto l_height = (lParam & 0xffff0000) >> 16;

			TVec2<uint32_t> l_newResolution = TVec2<uint32_t>((uint32_t)l_width, (uint32_t)l_height);
			g_Engine->Get<HIDService>()->WindowResizeCallback(l_newResolution.x, l_newResolution.y);
			
			return true;
		}

		return false;
	}
	case WM_KEYDOWN:
	{
		auto l_buttonState = new ButtonState((uint32_t)wParam, true);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});

		return true;
	}
	case WM_KEYUP:
	{
		auto l_buttonState = new ButtonState((uint32_t)wParam, false);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_LBUTTONDOWN:
	{
		auto l_buttonState = new ButtonState(INNO_MOUSE_BUTTON_LEFT, true);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_LBUTTONUP:
	{
		auto l_buttonState = new ButtonState(INNO_MOUSE_BUTTON_LEFT, false);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_RBUTTONDOWN:
	{
		auto l_buttonState = new ButtonState(INNO_MOUSE_BUTTON_RIGHT, true);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_RBUTTONUP:
	{
		auto l_buttonState = new ButtonState(INNO_MOUSE_BUTTON_RIGHT, false);
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_buttonState);
			});
		return true;
	}
	case WM_MOUSEMOVE:
	{
		auto l_mouseState = new MouseState(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		m_WindowEvents.Write([&](auto& l_BackBuffer)
			{
				l_BackBuffer.push_back(l_mouseState);
			});
		return true;
	}
	}
	return false;
}

bool WinWindowSystem::AddEventCallback(WindowEventCallback* callback)
{
	m_WindowEventCallbacks.emplace(callback);
	return true;
}

LPCSTR WinWindowSystem::GetApplicationName()
{
	return m_ApplicationName;
}

HINSTANCE WinWindowSystem::GetApplicationInstance()
{
	return m_ApplicationInstance;
}

HWND WinWindowSystem::GetWindowHandle()
{
	return m_WindowHandle;
}

bool WinWindowSystem::SetWindowHandle(HWND hwnd)
{
	m_WindowHandle = hwnd;
	return true;
}

LRESULT CALLBACK WinWindowSystem::WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto l_processed = g_Engine->getWindowSystem()->SendEvent(hwnd, uMsg, wParam, lParam);
	if (l_processed)
	{
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}