#include "DXWindowSystem.h"

#include "../component/WindowSystemComponent.h"
#include "../component/DXWindowSystemComponent.h"
#include "../component/DXRenderingSystemComponent.h"

#include "InputSystem.h"

#include "ICoreSystem.h"

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

private:
	windowCallbackWrapper() {};
};

static windowCallbackWrapper* ApplicationHandle = 0;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

INNO_PRIVATE_SCOPE DXWindowSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

IInputSystem* m_inputSystem;

static WindowSystemComponent* g_WindowSystemComponent;
static DXWindowSystemComponent* g_DXWindowSystemComponent;
}

INNO_SYSTEM_EXPORT bool DXWindowSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	DXWindowSystemNS::m_inputSystem = new InnoInputSystem();

	DXWindowSystemNS::g_WindowSystemComponent = &WindowSystemComponent::get();
	DXWindowSystemNS::g_DXWindowSystemComponent = &DXWindowSystemComponent::get();

	DXWindowSystemNS::g_WindowSystemComponent->m_windowName = g_pCoreSystem->getGameSystem()->getGameName();
	DXWindowSystemNS::g_DXWindowSystemComponent->m_hInstance = static_cast<HINSTANCE>(hInstance);
	DXWindowSystemNS::g_DXWindowSystemComponent->m_pScmdline = pScmdline;
	DXWindowSystemNS::g_DXWindowSystemComponent->m_nCmdshow = nCmdshow;

	WNDCLASS wc = {};

	// Get an external pointer to this object.	
	ApplicationHandle = &windowCallbackWrapper::get();

	// Give the application a name.
	auto l_windowName = DXWindowSystemNS::g_WindowSystemComponent->m_windowName;

	// Setup the windows class with default settings.
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = DXWindowSystemNS::g_DXWindowSystemComponent->m_hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = (LPCSTR)l_windowName.c_str();

	// Register the window class.
	RegisterClass(&wc);

	// Determine the resolution of the clients desktop screen.
	auto l_screenWidth = (int)DXWindowSystemNS::g_WindowSystemComponent->m_windowResolution.x;
	auto l_screenHeight = (int)DXWindowSystemNS::g_WindowSystemComponent->m_windowResolution.y;

	auto l_posX = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
	auto l_posY = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

	// Create the window with the screen settings and get the handle to it.
	DXWindowSystemNS::g_DXWindowSystemComponent->m_hwnd = CreateWindowEx(0, (LPCSTR)l_windowName.c_str(), (LPCSTR)l_windowName.c_str(),
		WS_OVERLAPPEDWINDOW,
		l_posX, l_posY, l_screenWidth, l_screenHeight, NULL, NULL, DXWindowSystemNS::g_DXWindowSystemComponent->m_hInstance, NULL);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(DXWindowSystemNS::g_DXWindowSystemComponent->m_hwnd, DXWindowSystemNS::g_DXWindowSystemComponent->m_nCmdshow);
	SetForegroundWindow(DXWindowSystemNS::g_DXWindowSystemComponent->m_hwnd);
	SetFocus(DXWindowSystemNS::g_DXWindowSystemComponent->m_hwnd);

	// Hide the mouse cursor.
	// ShowCursor(false);

	DXWindowSystemNS::m_inputSystem->setup();

	DXWindowSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool DXWindowSystem::initialize()
{
	DXWindowSystemNS::m_inputSystem->initialize();
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXWindowSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool DXWindowSystem::update()
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

	// If windows signals to end the application then exit out.
	if (msg.message == WM_QUIT)
	{
		DXWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	DXWindowSystemNS::m_inputSystem->update();
	return true;
}

INNO_SYSTEM_EXPORT bool DXWindowSystem::terminate()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if (DXWindowSystemNS::g_WindowSystemComponent->m_fullScreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(DXWindowSystemNS::g_DXWindowSystemComponent->m_hwnd);
	DXWindowSystemNS::g_DXWindowSystemComponent->m_hwnd = NULL;

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "DXWindowSystem: Window closed.");

	// Remove the application instance.
	UnregisterClass(DXWindowSystemNS::g_DXWindowSystemComponent->m_applicationName, DXWindowSystemNS::g_DXWindowSystemComponent->m_hInstance);
	DXWindowSystemNS::g_DXWindowSystemComponent->m_hInstance = NULL;

	// Release the pointer to this class.
	ApplicationHandle = NULL;

	DXWindowSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXWindowSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus DXWindowSystem::getStatus()
{
	return DXWindowSystemNS::m_objectStatus;
}

void DXWindowSystem::swapBuffer()
{
	// Present the back buffer to the screen since rendering is complete.
	if (DXRenderingSystemComponent::get().m_vsync_enabled)
	{
		// Lock to screen refresh rate.
		DXRenderingSystemComponent::get().m_swapChain->Present(1, 0);
	}
	else
	{
		// Present as fast as possible.
		DXRenderingSystemComponent::get().m_swapChain->Present(0, 0);
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
		EndPaint(hwnd, &ps);
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
	ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam);

	switch (umsg)
	{
	case WM_KEYDOWN:
	{
		auto l_result = DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find((int)wparam);
		if (l_result != DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::PRESSED;
		}
		return 0;
	}
	case WM_KEYUP:
	{
		auto l_result = DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find((int)wparam);
		if (l_result != DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::RELEASED;
		}

		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		auto l_result = DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(INNO_MOUSE_BUTTON_LEFT);
		if (l_result != DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::PRESSED;
		}
		return 0;
	}
	case WM_LBUTTONUP:
	{
		auto l_result = DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(INNO_MOUSE_BUTTON_LEFT);
		if (l_result != DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::RELEASED;
		}
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		auto l_result = DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(INNO_MOUSE_BUTTON_RIGHT);
		if (l_result != DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::PRESSED;
		}
		return 0;
	}
	case WM_RBUTTONUP:
	{
		auto l_result = DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(INNO_MOUSE_BUTTON_RIGHT);
		if (l_result != DXWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
		{
			l_result->second = ButtonStatus::RELEASED;
		}
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		auto l_mouseCurrentX = GET_X_LPARAM(lparam);
		auto l_mouseCurrentY = GET_Y_LPARAM(lparam);
		DXWindowSystemNS::m_inputSystem->mousePositionCallback((float)l_mouseCurrentX, (float)l_mouseCurrentY);
		return 0;
	}
	// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}
