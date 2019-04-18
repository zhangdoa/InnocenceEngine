#include "WinWindowSystem.h"
#include "../../component/WinWindowSystemComponent.h"

#include "WinDXWindow/WinDXWindowSystem.h"
#include "WinGLWindow/WinGLWindowSystem.h"
//#include "WinVKWindow/WinVKWindowSystem.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

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

	ButtonStatusMap m_buttonStatus;

private:
	windowCallbackWrapper() {};
};

static windowCallbackWrapper* ApplicationHandle = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INNO_PRIVATE_SCOPE WinWindowSystemNS
{
	IWinWindowSystem* m_backendWindowSystem;
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	InitConfig m_initConfig;
}

bool WinWindowSystem::setup(void* hInstance, void* hwnd)
{
	for (int i = 0; i < g_pCoreSystem->getInputSystem()->getInputConfig().totalKeyCodes; i++)
	{
		windowCallbackWrapper::get().m_buttonStatus.emplace(i, ButtonStatus::RELEASED);
	}

	WinWindowSystemComponent::get().m_hInstance = static_cast<HINSTANCE>(hInstance);

	if (hwnd)
	{
		WinWindowSystemComponent::get().m_hwnd = *reinterpret_cast<HWND*>(hwnd);
	}

	WinWindowSystemComponent::get().m_applicationName = "InnocenceEngineWindow";

	ApplicationHandle = &windowCallbackWrapper::get();

	// create window surface for different rendering backend
	WinWindowSystemNS::m_initConfig = g_pCoreSystem->getVisionSystem()->getInitConfig();

	switch (WinWindowSystemNS::m_initConfig.renderingBackend)
	{
	case RenderingBackend::GL:
		WinWindowSystemNS::m_backendWindowSystem = new WinGLWindowSystem();
		break;
	case RenderingBackend::DX11:
#if defined INNO_PLATFORM_WIN
		WinWindowSystemNS::m_backendWindowSystem = new WinDXWindowSystem();
#endif
		break;
	case RenderingBackend::DX12:
#if defined INNO_PLATFORM_WIN
		//WinWindowSystemNS::m_backendWindowSystem = new WinDXWindowSystem();
#endif
		break;
	case RenderingBackend::VK:
#if defined INNO_RENDERER_VULKAN
		//WinWindowSystemNS::m_backendWindowSystem = new WinVKWindowSystem();
#endif
		break;
	default:
		break;
	}

	WinWindowSystemNS::m_backendWindowSystem->setup(WinWindowSystemComponent::get().m_hInstance, WinWindowSystemComponent::get().m_hwnd, WindowProc);

	WinWindowSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinWindowSystem setup finished.");

	return true;
}

bool WinWindowSystem::initialize()
{
	WinWindowSystemNS::m_backendWindowSystem->initialize();
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinWindowSystem has been initialized.");
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
	WinWindowSystemNS::m_backendWindowSystem->terminate();

	if (WinWindowSystemNS::m_initConfig.engineMode == EngineMode::GAME)
	{
		// Show the mouse cursor.
		ShowCursor(true);

		// Remove the window.
		DestroyWindow(WinWindowSystemComponent::get().m_hwnd);
		WinWindowSystemComponent::get().m_hwnd = NULL;

		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "WinWindowSystem: Window closed.");

		// Remove the application instance.
		UnregisterClass(WinWindowSystemComponent::get().m_applicationName, WinWindowSystemComponent::get().m_hInstance);
		WinWindowSystemComponent::get().m_hInstance = NULL;

		// Release the pointer to this class.
		ApplicationHandle = NULL;
	}

	PostQuitMessage(0);

	WinWindowSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinWindowSystem has been terminated.");
	return true;
}

ObjectStatus WinWindowSystem::getStatus()
{
	return WinWindowSystemNS::m_objectStatus;
}

ButtonStatusMap WinWindowSystem::getButtonStatus()
{
	return windowCallbackWrapper::get().m_buttonStatus;
}

bool WinWindowSystem::sendEvent(unsigned int umsg, unsigned int WParam, int LParam)
{
	windowCallbackWrapper::get().MessageHandler(0, umsg, WParam, LParam);
	return true;
}

void WinWindowSystem::swapBuffer()
{
	WinWindowSystemNS::m_backendWindowSystem->swapBuffer();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hwnd != WinWindowSystemComponent::get().m_hwnd)
	{
		return ApplicationHandle->MessageHandler(hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_DESTROY:
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "WinWindowSystem: WM_DESTROY signal received.");
		WinWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return 0;
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
		if (lParam && g_pCoreSystem->getStatus() == ObjectStatus::ALIVE)
		{
			auto l_width = lParam & 0xffff;
			auto l_height = (lParam & 0xffff0000) >> 16;

			TVec2<unsigned int> l_newResolution = TVec2<unsigned int>((unsigned int)l_width, (unsigned int)l_height);
			g_pCoreSystem->getVisionSystem()->getRenderingFrontend()->setScreenResolution(l_newResolution);
			g_pCoreSystem->getVisionSystem()->getRenderingBackend()->resize();
		}
	}
	default:
	{
		return ApplicationHandle->MessageHandler(hwnd, uMsg, wParam, lParam);
	}
	}
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT windowCallbackWrapper::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	if (WinWindowSystemNS::m_initConfig.engineMode == EngineMode::GAME)
	{
		ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam);
	}

	switch (umsg)
	{
	case WM_KEYDOWN:
	{
		auto l_result = m_buttonStatus.find((int)wparam);
		if (l_result != m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::PRESSED;
		}
		return 0;
	}
	case WM_KEYUP:
	{
		auto l_result = m_buttonStatus.find((int)wparam);
		if (l_result != m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::RELEASED;
		}

		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		auto l_result = m_buttonStatus.find(INNO_MOUSE_BUTTON_LEFT);
		if (l_result != m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::PRESSED;
		}
		return 0;
	}
	case WM_LBUTTONUP:
	{
		auto l_result = m_buttonStatus.find(INNO_MOUSE_BUTTON_LEFT);
		if (l_result != m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::RELEASED;
		}
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		auto l_result = m_buttonStatus.find(INNO_MOUSE_BUTTON_RIGHT);
		if (l_result != m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::PRESSED;
		}
		return 0;
	}
	case WM_RBUTTONUP:
	{
		auto l_result = m_buttonStatus.find(INNO_MOUSE_BUTTON_RIGHT);
		if (l_result != m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::RELEASED;
		}
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		auto l_mouseCurrentX = GET_X_LPARAM(lparam);
		auto l_mouseCurrentY = GET_Y_LPARAM(lparam);
		g_pCoreSystem->getInputSystem()->mousePositionCallback((float)l_mouseCurrentX, (float)l_mouseCurrentY);
		return 0;
	}
	// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}