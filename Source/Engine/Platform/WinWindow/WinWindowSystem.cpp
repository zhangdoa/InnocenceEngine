#include "WinWindowSystem.h"
#include "../../Common/LogService.h"
#include "../../Services/HIDService.h"
#include "../../Services/RenderingFrontend.h"

#include "DXWindowSurface/WinDXWindowSurface.h"
#include "GLWindowSurface/WinGLWindowSurface.h"
#include "VKWindowSurface/WinVKWindowSurface.h"

#include "../../Engine.h"

using namespace Inno;
;

namespace WinWindowSystemNS
{
	LRESULT CALLBACK ProcessEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	IWindowSurface* m_WindowSurface;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_InitConfig;

	std::vector<ButtonState> m_ButtonStates;
	std::set<WindowEventCallback*> m_WindowEventCallbacks;

	HINSTANCE m_ApplicationInstance;
	LPCSTR m_ApplicationName;
	HWND m_WindowHandle;
};

using namespace WinWindowSystemNS;

LRESULT WinWindowSystemNS::ProcessEvent(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	for (auto i : m_WindowEventCallbacks)
	{
		(*i)(hwnd, uMsg, (uint64_t)wParam, (int64_t)lParam);
	}

	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		m_ButtonStates[(uint64_t)wParam].m_isPressed = true;
		return 0;
	}
	case WM_KEYUP:
	{
		m_ButtonStates[(uint64_t)wParam].m_isPressed = false;
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		m_ButtonStates[INNO_MOUSE_BUTTON_LEFT].m_isPressed = true;
		return 0;
	}
	case WM_LBUTTONUP:
	{
		m_ButtonStates[INNO_MOUSE_BUTTON_LEFT].m_isPressed = false;

		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		m_ButtonStates[INNO_MOUSE_BUTTON_RIGHT].m_isPressed = true;
		return 0;
	}
	case WM_RBUTTONUP:
	{
		m_ButtonStates[INNO_MOUSE_BUTTON_RIGHT].m_isPressed = false;
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		auto l_mouseCurrentX = GET_X_LPARAM(lParam);
		auto l_mouseCurrentY = GET_Y_LPARAM(lParam);
		g_Engine->Get<HIDService>()->MouseMovementCallback((float)l_mouseCurrentX, (float)l_mouseCurrentY);
		return 0;
	}
	// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
}

bool WinWindowSystem::Setup(ISystemConfig* systemConfig)
{
	m_ButtonStates.resize(g_Engine->Get<HIDService>()->GetInputConfig().totalKeyCodes);
	for (size_t i = 0; i < m_ButtonStates.size(); i++)
	{
		m_ButtonStates[i].m_code = (uint32_t)i;
	}

	auto l_systemConfig = reinterpret_cast<IWindowSystemConfig*>(systemConfig);
	m_ApplicationInstance = static_cast<HINSTANCE>(l_systemConfig->m_AppHook);
	if (l_systemConfig->m_ExtraHook)
	{
		m_WindowHandle = *reinterpret_cast<HWND*>(l_systemConfig->m_ExtraHook);
	}

	m_ApplicationName = g_Engine->GetApplicationName().c_str();
	WinWindowSystemNS::m_InitConfig = g_Engine->getInitConfig();
	switch (WinWindowSystemNS::m_InitConfig.renderingServer)
	{
	case RenderingServer::GL:
		WinWindowSystemNS::m_WindowSurface = new WinGLWindowSurface();
		break;
	case RenderingServer::DX11:
#if defined INNO_PLATFORM_WIN
		WinWindowSystemNS::m_WindowSurface = new WinDXWindowSurface();
#endif
		break;
	case RenderingServer::DX12:
#if defined INNO_PLATFORM_WIN
		WinWindowSystemNS::m_WindowSurface = new WinDXWindowSurface();
#endif
		break;
	case RenderingServer::VK:
#if defined INNO_RENDERER_VULKAN
		WinWindowSystemNS::m_WindowSurface = new WinVKWindowSurface();
#endif
		break;
	default:
		break;
	}

	IWindowSurfaceConfig l_surfaceConfig;
	l_surfaceConfig.hInstance = m_ApplicationInstance;
	l_surfaceConfig.hwnd = m_WindowHandle;
	l_surfaceConfig.WindowProc = WinWindowSystemNS::WindowProcedure;

	WinWindowSystemNS::m_WindowSurface->Setup(&l_surfaceConfig);

	WinWindowSystemNS::m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "WinWindowSystem Setup finished.");

	return true;
}

bool WinWindowSystem::Initialize()
{
	WinWindowSystemNS::m_WindowSurface->Initialize();
	Log(Success, "WinWindowSystem has been initialized.");
	return true;
}

bool WinWindowSystem::Update()
{
	if (WinWindowSystemNS::m_InitConfig.engineMode == EngineMode::Host)
	{
		MSG msg;

		// Initialize the message structure.
		ZeroMemory(&msg, sizeof(MSG));

		// Handle the windows messages.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return true;
}

bool WinWindowSystem::Terminate()
{
	WinWindowSystemNS::m_WindowSurface->Terminate();

	if (WinWindowSystemNS::m_InitConfig.engineMode == EngineMode::Host)
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

	WinWindowSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "WinWindowSystem has been terminated.");
	return true;
}

ObjectStatus WinWindowSystem::GetStatus()
{
	return WinWindowSystemNS::m_ObjectStatus;
}

IWindowSurface* WinWindowSystem::GetWindowSurface()
{
	return WinWindowSystemNS::m_WindowSurface;
}

const std::vector<ButtonState>& WinWindowSystem::GetButtonState()
{
	return m_ButtonStates;
}

bool WinWindowSystem::SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
	WinWindowSystemNS::WindowProcedure(m_WindowHandle, uMsg, wParam, lParam);
	return true;
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

LRESULT CALLBACK WinWindowSystemNS::WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// For to eliminate fake OpenGL window handle event
	if (hwnd != m_WindowHandle)
		return ProcessEvent(hwnd, uMsg, wParam, lParam);

	switch (uMsg)
	{
	case WM_DESTROY:
	{
		Log(Warning, "WM_DESTROY signal received.");
		WinWindowSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
	}
	case WM_SIZE:
	{
		if (lParam && g_Engine->GetStatus() == ObjectStatus::Activated)
		{
			auto l_width = lParam & 0xffff;
			auto l_height = (lParam & 0xffff0000) >> 16;

			TVec2<uint32_t> l_newResolution = TVec2<uint32_t>((uint32_t)l_width, (uint32_t)l_height);
			g_Engine->Get<RenderingFrontend>()->SetScreenResolution(l_newResolution);
			g_Engine->getRenderingServer()->Resize();
		}
	}
	default:
	{
		return ProcessEvent(hwnd, uMsg, wParam, lParam);
	}
	}
}