#include "DXWindowSystem.h"

#if defined(INNO_RENDERER_DX)
void DXWindowSystem::setup(void* appInstance, char* commandLineArg, int showMethod)
{
	WNDCLASS wc = {};

	// Get an external pointer to this object.	
	ApplicationHandle = this;

	// Get the instance of this application.
	m_hinstance = (HINSTANCE)appInstance;

	// Give the application a name.
	const wchar_t CLASS_NAME[] = L"Sample Window Class";

	// Setup the windows class with default settings.
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hinstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = (LPCSTR)CLASS_NAME;

	// Register the window class.
	RegisterClass(&wc);

	// Determine the resolution of the clients desktop screen.
	auto l_screenWidth = (int)WindowSystemSingletonComponent::getInstance().m_windowResolution.x;
	auto l_screenHeight = (int)WindowSystemSingletonComponent::getInstance().m_windowResolution.y;

	auto l_posX = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
	auto l_posY = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

	// Create the window with the screen settings and get the handle to it.
	m_hwnd = CreateWindowEx(0, (LPCSTR)CLASS_NAME, (LPCSTR)CLASS_NAME,
		WS_OVERLAPPEDWINDOW,
		l_posX, l_posY, l_screenWidth, l_screenHeight, NULL, NULL, m_hinstance, NULL);

	// Bring the window up on the screen and set it as main focus.
	ShowWindow(m_hwnd, showMethod);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	// Hide the mouse cursor.
	ShowCursor(false);

	BaseWindowSystem::setup();

	m_objectStatus = objectStatus::ALIVE;
}
#endif

void DXWindowSystem::initialize()
{
	//initialize window

	//initialize input

	BaseWindowSystem::initialize();

	g_pLogSystem->printLog("DXWindowSystem has been initialized.");
}

void DXWindowSystem::update()
{
	//update window
	MSG msg;

	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));

	// Loop until there is a quit message from the window or the user.
	while (m_objectStatus == objectStatus::ALIVE)
	{
		// Handle the windows messages.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT)
		{
			m_objectStatus = objectStatus::STANDBY;
		}
		else
		{
		}
	}

	//update input
	updateInput();

	BaseWindowSystem::update();
}

void DXWindowSystem::shutdown()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if (WindowSystemSingletonComponent::getInstance().m_fullScreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	g_pLogSystem->printLog("DXWindowSystem: Window closed.");

	// Remove the application instance.
	UnregisterClass(m_applicationName, m_hinstance);
	m_hinstance = NULL;

	// Release the pointer to this class.
	ApplicationHandle = NULL;

	m_objectStatus = objectStatus::SHUTDOWN;
	g_pLogSystem->printLog("DXWindowSystem has been shutdown.");
}

const objectStatus & DXWindowSystem::getStatus() const
{
	return m_objectStatus;
}

void DXWindowSystem::swapBuffer()
{
}

LRESULT DXWindowSystem::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
	case WM_KEYDOWN:
	{
		return 0;
	}

	// Check if a key has been released on the keyboard.
	case WM_KEYUP:
	{
		return 0;
	}

	// Any other messages send to the default message handler as our application won't make use of them.
	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}
}

void DXWindowSystem::updateInput()
{
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
