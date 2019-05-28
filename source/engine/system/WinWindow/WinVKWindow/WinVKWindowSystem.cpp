#include "WinVKWindowSystem.h"
#include "../../../component/WinWindowSystemComponent.h"
#include "../../../component/VKRenderingSystemComponent.h"

#include "../../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE WinVKWindowSystemNS
{
	bool setup(void* hInstance, void* hwnd, void* WindowProc);
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
}

bool WinVKWindowSystemNS::setup(void* hInstance, void* hwnd, void* WindowProc)
{
	m_initConfig = g_pCoreSystem->getInitConfig();

	// Setup the windows class with default settings.
	WNDCLASS wc = {};

	auto l_windowName = g_pCoreSystem->getGameSystem()->getGameName();

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = WinWindowSystemComponent::get().m_hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszClassName = WinWindowSystemComponent::get().m_applicationName;

	// Register the window class.
	RegisterClass(&wc);

	// Determine the resolution of the clients desktop screen.
	auto l_screenResolution = g_pCoreSystem->getRenderingFrontendSystem()->getScreenResolution();
	auto l_screenWidth = (int)l_screenResolution.x;
	auto l_screenHeight = (int)l_screenResolution.y;

	auto l_posX = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
	auto l_posY = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

	if (m_initConfig.engineMode == EngineMode::GAME)
	{
		// Create the window with the screen settings and get the handle to it.
		WinWindowSystemComponent::get().m_hwnd = CreateWindowEx(0, WinWindowSystemComponent::get().m_applicationName, (LPCSTR)l_windowName.c_str(),
			WS_OVERLAPPEDWINDOW,
			l_posX, l_posY, l_screenWidth, l_screenHeight, NULL, NULL, WinWindowSystemComponent::get().m_hInstance, NULL);
	}

	WinWindowSystemComponent::get().m_HDC = GetDC(WinWindowSystemComponent::get().m_hwnd);

	m_objectStatus = ObjectStatus::Activated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinVKWindowSystem setup finished.");

	return true;
}

bool WinVKWindowSystemNS::initialize()
{
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.hinstance = WinWindowSystemComponent::get().m_hInstance;
	createInfo.hwnd = WinWindowSystemComponent::get().m_hwnd;

	if (vkCreateWin32SurfaceKHR(VKRenderingSystemComponent::get().m_instance, &createInfo, NULL, &VKRenderingSystemComponent::get().m_windowSurface) != VK_SUCCESS)
	{
		m_objectStatus = ObjectStatus::Created;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "WinVKWindowSystem: Failed to create window surface!");
		return false;
	}

	if (m_initConfig.engineMode == EngineMode::GAME)
	{
		// Bring the window up on the screen and set it as main focus.
		ShowWindow(WinWindowSystemComponent::get().m_hwnd, true);
		SetForegroundWindow(WinWindowSystemComponent::get().m_hwnd);
		SetFocus(WinWindowSystemComponent::get().m_hwnd);
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinVKWindowSystem has been initialized.");
	return true;
}

bool WinVKWindowSystemNS::update()
{
	return true;
}

bool WinVKWindowSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinVKWindowSystemNS has been terminated.");

	return true;
}

bool WinVKWindowSystem::setup(void* hInstance, void* hwnd, void* WindowProc)
{
	return WinVKWindowSystemNS::setup(hInstance, hwnd, WindowProc);
}

bool WinVKWindowSystem::initialize()
{
	return WinVKWindowSystemNS::initialize();
}

bool WinVKWindowSystem::update()
{
	return WinVKWindowSystemNS::update();
}

bool WinVKWindowSystem::terminate()
{
	return WinVKWindowSystemNS::terminate();
}

ObjectStatus WinVKWindowSystem::getStatus()
{
	return WinVKWindowSystemNS::m_objectStatus;
}

void WinVKWindowSystem::swapBuffer()
{
}