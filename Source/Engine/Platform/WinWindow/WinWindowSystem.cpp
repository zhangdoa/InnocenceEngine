#include "WinWindowSystem.h"
#include "../../Core/Logger.h"

#include "DXWindowSurface/WinDXWindowSurface.h"
#include "GLWindowSurface/WinGLWindowSurface.h"
#include "VKWindowSurface/WinVKWindowSurface.h"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace WinWindowSystemNS
{
	LRESULT CALLBACK MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

	IWindowSurface* m_windowSurface;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;

	std::vector<ButtonState> m_buttonState;
	std::set<WindowEventCallbackFunctor*> m_windowEventCallbackFunctor;

	HINSTANCE m_hInstance;
	LPCSTR m_applicationName;
	HWND m_hwnd;
};

using namespace WinWindowSystemNS;

LRESULT WinWindowSystemNS::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	for (auto i : m_windowEventCallbackFunctor)
	{
		(*i)(hwnd, umsg, (uint64_t)wparam, (int64_t)lparam);
	}

	switch (umsg)
	{
	case WM_KEYDOWN:
	{
		m_buttonState[(uint64_t)wparam].m_isPressed = true;
		return 0;
	}
	case WM_KEYUP:
	{
		m_buttonState[(uint64_t)wparam].m_isPressed = false;
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		m_buttonState[INNO_MOUSE_BUTTON_LEFT].m_isPressed = true;
		return 0;
	}
	case WM_LBUTTONUP:
	{
		m_buttonState[INNO_MOUSE_BUTTON_LEFT].m_isPressed = false;

		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		m_buttonState[INNO_MOUSE_BUTTON_RIGHT].m_isPressed = true;
		return 0;
	}
	case WM_RBUTTONUP:
	{
		m_buttonState[INNO_MOUSE_BUTTON_RIGHT].m_isPressed = false;
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		auto l_mouseCurrentX = GET_X_LPARAM(lparam);
		auto l_mouseCurrentY = GET_Y_LPARAM(lparam);
		g_Engine->getEventSystem()->mouseMovementCallback((float)l_mouseCurrentX, (float)l_mouseCurrentY);
		return 0;
	}
	// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}

bool WinWindowSystem::Setup(ISystemConfig* systemConfig)
{
	auto l_systemConfig = reinterpret_cast<IWindowSystemConfig*>(systemConfig);

	m_buttonState.resize(g_Engine->getEventSystem()->getInputConfig().totalKeyCodes);

	for (size_t i = 0; i < m_buttonState.size(); i++)
	{
		m_buttonState[i].m_code = (uint32_t)i;
	}

	m_hInstance = static_cast<HINSTANCE>(l_systemConfig->m_AppHook);

	if (l_systemConfig->m_ExtraHook)
	{
		m_hwnd = *reinterpret_cast<HWND*>(l_systemConfig->m_ExtraHook);
	}

	m_applicationName = g_Engine->getApplicationName().c_str();

	// create window surface for different rendering backend
	WinWindowSystemNS::m_initConfig = g_Engine->getInitConfig();

	switch (WinWindowSystemNS::m_initConfig.renderingServer)
	{
	case RenderingServer::GL:
		WinWindowSystemNS::m_windowSurface = new WinGLWindowSurface();
		break;
	case RenderingServer::DX11:
#if defined INNO_PLATFORM_WIN
		WinWindowSystemNS::m_windowSurface = new WinDXWindowSurface();
#endif
		break;
	case RenderingServer::DX12:
#if defined INNO_PLATFORM_WIN
		WinWindowSystemNS::m_windowSurface = new WinDXWindowSurface();
#endif
		break;
	case RenderingServer::VK:
#if defined INNO_RENDERER_VULKAN
		WinWindowSystemNS::m_windowSurface = new WinVKWindowSurface();
#endif
		break;
	default:
		break;
	}

	IWindowSurfaceConfig l_surfaceConfig;
	l_surfaceConfig.hInstance = m_hInstance;
	l_surfaceConfig.hwnd = m_hwnd;
	l_surfaceConfig.WindowProc = WindowProc;

	WinWindowSystemNS::m_windowSurface->Setup(&l_surfaceConfig);

	WinWindowSystemNS::m_ObjectStatus = ObjectStatus::Activated;
	Logger::Log(LogLevel::Success, "WinWindowSystem Setup finished.");

	return true;
}

bool WinWindowSystem::Initialize()
{
	WinWindowSystemNS::m_windowSurface->Initialize();
	Logger::Log(LogLevel::Success, "WinWindowSystem has been initialized.");
	return true;
}

bool WinWindowSystem::Update()
{
	if (WinWindowSystemNS::m_initConfig.engineMode == EngineMode::Host)
	{
		//Update window
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
	WinWindowSystemNS::m_windowSurface->Terminate();

	if (WinWindowSystemNS::m_initConfig.engineMode == EngineMode::Host)
	{
		// Show the mouse cursor.
		ShowCursor(true);

		// Remove the window.
		DestroyWindow(m_hwnd);
		m_hwnd = NULL;

		Logger::Log(LogLevel::Warning, "WinWindowSystem: Window closed.");

		// Remove the application instance.
		UnregisterClass(m_applicationName, m_hInstance);
		m_hInstance = NULL;
	}

	PostQuitMessage(0);

	WinWindowSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	Logger::Log(LogLevel::Success, "WinWindowSystem has been terminated.");
	return true;
}

ObjectStatus WinWindowSystem::GetStatus()
{
	return WinWindowSystemNS::m_ObjectStatus;
}

IWindowSurface* WinWindowSystem::getWindowSurface()
{
	return WinWindowSystemNS::m_windowSurface;
}

const std::vector<ButtonState>& WinWindowSystem::getButtonState()
{
	return m_buttonState;
}

bool WinWindowSystem::sendEvent(uint32_t umsg, uint32_t WParam, int32_t LParam)
{
	WindowProc(m_hwnd, umsg, WParam, LParam);
	return true;
}

bool WinWindowSystem::addEventCallback(WindowEventCallbackFunctor* functor)
{
	m_windowEventCallbackFunctor.emplace(functor);
	return true;
}

LPCSTR WinWindowSystem::getApplicationName()
{
	return m_applicationName;
}

HINSTANCE WinWindowSystem::getHInstance()
{
	return m_hInstance;
}

HWND WinWindowSystem::getHwnd()
{
	return m_hwnd;
}

bool WinWindowSystem::setHwnd(HWND rhs)
{
	m_hwnd = rhs;
	return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// For to eliminate fake OpenGL window handle event
	if (hwnd != m_hwnd)
	{
		return MessageHandler(hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_DESTROY:
	{
		Logger::Log(LogLevel::Warning, "WinWindowSystem: WM_DESTROY signal received.");
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
			g_Engine->getRenderingFrontend()->SetScreenResolution(l_newResolution);
			g_Engine->getRenderingServer()->Resize();
		}
	}
	default:
	{
		return MessageHandler(hwnd, uMsg, wParam, lParam);
	}
	}
}