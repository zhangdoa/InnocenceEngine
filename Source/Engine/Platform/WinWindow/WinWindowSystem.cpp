#include "WinWindowSystem.h"
#include "../../Component/WinWindowSystemComponent.h"
#include "../../Core/InnoLogger.h"

#include "DXWindowSurface/WinDXWindowSurface.h"
#include "GLWindowSurface/WinGLWindowSurface.h"
#include "VKWindowSurface/WinVKWindowSurface.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

class windowCallbackWrapper
{
public:
	~windowCallbackWrapper() {};

	static windowCallbackWrapper& get()
	{
		static windowCallbackWrapper instance;
		return instance;
	}

	LRESULT CALLBACK MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

private:
	windowCallbackWrapper() {};
};

static windowCallbackWrapper* ApplicationHandle = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace WinWindowSystemNS
{
	IWindowSurface* m_windowSurface;
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
	std::vector<ButtonState> m_buttonState;
}

using namespace WinWindowSystemNS;

bool WinWindowSystem::setup(void* hInstance, void* hwnd)
{
	m_buttonState.resize(g_pModuleManager->getEventSystem()->getInputConfig().totalKeyCodes);

	for (size_t i = 0; i < m_buttonState.size(); i++)
	{
		m_buttonState[i].m_code = (unsigned int)i;
	}

	WinWindowSystemComponent::get().m_hInstance = static_cast<HINSTANCE>(hInstance);

	if (hwnd)
	{
		WinWindowSystemComponent::get().m_hwnd = *reinterpret_cast<HWND*>(hwnd);
	}

	WinWindowSystemComponent::get().m_applicationName = "InnocenceEngineWindow";

	ApplicationHandle = &windowCallbackWrapper::get();

	// create window surface for different rendering backend
	WinWindowSystemNS::m_initConfig = g_pModuleManager->getInitConfig();

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

	WinWindowSystemNS::m_windowSurface->setup(WinWindowSystemComponent::get().m_hInstance, WinWindowSystemComponent::get().m_hwnd, WindowProc);

	WinWindowSystemNS::m_objectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "WinWindowSystem setup finished.");

	return true;
}

bool WinWindowSystem::initialize()
{
	WinWindowSystemNS::m_windowSurface->initialize();
	InnoLogger::Log(LogLevel::Success, "WinWindowSystem has been initialized.");
	return true;
}

bool WinWindowSystem::update()
{
	if (WinWindowSystemNS::m_initConfig.engineMode == EngineMode::GAME)
	{
		//update window
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

bool WinWindowSystem::terminate()
{
	WinWindowSystemNS::m_windowSurface->terminate();

	if (WinWindowSystemNS::m_initConfig.engineMode == EngineMode::GAME)
	{
		// Show the mouse cursor.
		ShowCursor(true);

		// Remove the window.
		DestroyWindow(WinWindowSystemComponent::get().m_hwnd);
		WinWindowSystemComponent::get().m_hwnd = NULL;

		InnoLogger::Log(LogLevel::Warning, "WinWindowSystem: Window closed.");

		// Remove the application instance.
		UnregisterClass(WinWindowSystemComponent::get().m_applicationName, WinWindowSystemComponent::get().m_hInstance);
		WinWindowSystemComponent::get().m_hInstance = NULL;

		// Release the pointer to this class.
		ApplicationHandle = NULL;
	}

	PostQuitMessage(0);

	WinWindowSystemNS::m_objectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "WinWindowSystem has been terminated.");
	return true;
}

ObjectStatus WinWindowSystem::getStatus()
{
	return WinWindowSystemNS::m_objectStatus;
}

IWindowSurface * WinWindowSystem::getWindowSurface()
{
	return WinWindowSystemNS::m_windowSurface;
}

const std::vector<ButtonState>& WinWindowSystem::getButtonState()
{
	return m_buttonState;
}

bool WinWindowSystem::sendEvent(unsigned int umsg, unsigned int WParam, int LParam)
{
	WindowProc(WinWindowSystemComponent::get().m_hwnd, umsg, WParam, LParam);
	return true;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// For to eliminate fake OpenGL window handle event
	if (hwnd != WinWindowSystemComponent::get().m_hwnd)
	{
		return ApplicationHandle->MessageHandler(hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_DESTROY:
	{
		InnoLogger::Log(LogLevel::Warning, "WinWindowSystem: WM_DESTROY signal received.");
		WinWindowSystemNS::m_objectStatus = ObjectStatus::Suspended;
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
		if (lParam && g_pModuleManager->getStatus() == ObjectStatus::Activated)
		{
			auto l_width = lParam & 0xffff;
			auto l_height = (lParam & 0xffff0000) >> 16;

			TVec2<unsigned int> l_newResolution = TVec2<unsigned int>((unsigned int)l_width, (unsigned int)l_height);
			g_pModuleManager->getRenderingFrontend()->setScreenResolution(l_newResolution);
			g_pModuleManager->getRenderingServer()->Resize();
		}
	}
	default:
	{
		return ApplicationHandle->MessageHandler(hwnd, uMsg, wParam, lParam);
	}
	}
}

//extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT windowCallbackWrapper::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if (WinWindowSystemNS::m_initConfig.engineMode == EngineMode::GAME)
	{
		//ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam);
	}

	switch (umsg)
	{
	case WM_KEYDOWN:
	{
		//g_pModuleManager->getEventSystem()->buttonStatusCallback(ButtonData{ (int)wparam, true });
		m_buttonState[(int)wparam].m_isPressed = true;
		return 0;
	}
	case WM_KEYUP:
	{
		//g_pModuleManager->getEventSystem()->buttonStatusCallback(ButtonData{ (int)wparam, ButtonStatus::Released });
		m_buttonState[(int)wparam].m_isPressed = false;
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		//g_pModuleManager->getEventSystem()->buttonStatusCallback(ButtonData{ INNO_MOUSE_BUTTON_LEFT, true });
		m_buttonState[INNO_MOUSE_BUTTON_LEFT].m_isPressed = true;
		return 0;
	}
	case WM_LBUTTONUP:
	{
		//g_pModuleManager->getEventSystem()->buttonStatusCallback(ButtonData{ INNO_MOUSE_BUTTON_LEFT, ButtonStatus::Released });
		m_buttonState[INNO_MOUSE_BUTTON_LEFT].m_isPressed = false;

		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		//g_pModuleManager->getEventSystem()->buttonStatusCallback(ButtonData{ INNO_MOUSE_BUTTON_RIGHT, true });
		m_buttonState[INNO_MOUSE_BUTTON_RIGHT].m_isPressed = true;
		return 0;
	}
	case WM_RBUTTONUP:
	{
		//g_pModuleManager->getEventSystem()->buttonStatusCallback(ButtonData{ INNO_MOUSE_BUTTON_RIGHT, ButtonStatus::Released });
		m_buttonState[INNO_MOUSE_BUTTON_RIGHT].m_isPressed = false;
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		auto l_mouseCurrentX = GET_X_LPARAM(lparam);
		auto l_mouseCurrentY = GET_Y_LPARAM(lparam);
		g_pModuleManager->getEventSystem()->mousePositionCallback((float)l_mouseCurrentX, (float)l_mouseCurrentY);
		return 0;
	}
	// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}