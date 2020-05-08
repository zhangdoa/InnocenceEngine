#include "WinVKWindowSurface.h"
#include "../WinWindowSystem.h"
#include "../../Engine/Core/InnoLogger.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "../../../RenderingServer/VK/VKRenderingServer.h"
#include "../../../Interface/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace WinVKWindowSurfaceNS
{
	bool setup(void* hInstance, void* hwnd, void* WindowProc);
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
}

bool WinVKWindowSurfaceNS::setup(void* hInstance, void* hwnd, void* WindowProc)
{
	m_initConfig = g_pModuleManager->getInitConfig();

	// Setup the windows class with default settings.
	auto l_windowName = g_pModuleManager->getApplicationName();

	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = (WNDPROC)WindowProc;
	wcex.hInstance = reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHInstance();
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getApplicationName();

	auto l_windowClass = MAKEINTATOM(RegisterClassEx(&wcex));

	// Determine the resolution of the clients desktop screen.
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_screenWidth = (int32_t)l_screenResolution.x;
	auto l_screenHeight = (int32_t)l_screenResolution.y;

	auto l_posX = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
	auto l_posY = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

	if (m_initConfig.engineMode == EngineMode::Host)
	{
		// create a new window and context
		auto l_hwnd = CreateWindow(
			l_windowClass, reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getApplicationName(), // class name, window name
			WS_OVERLAPPEDWINDOW, // styles
			l_posX, l_posY, // posx, posy. If x is set to CW_USEDEFAULT y is ignored
			l_screenWidth, l_screenHeight, // width, height
			NULL, NULL, // parent window, menu
			reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHInstance(), NULL); // instance, param

		reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->setHwnd(l_hwnd);
	}

	m_ObjectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "WinVKWindowSurface setup finished.");

	return true;
}

bool WinVKWindowSurfaceNS::initialize()
{
	VkWin32SurfaceCreateInfoKHR l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	l_createInfo.pNext = NULL;
	l_createInfo.hinstance = reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHInstance();
	l_createInfo.hwnd = reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHwnd();

	auto l_renderingServer = reinterpret_cast<VKRenderingServer*>(g_pModuleManager->getRenderingServer());
	auto l_VkInstance = reinterpret_cast<VkInstance>(l_renderingServer->GetVkInstance());
	auto l_VkSurface = reinterpret_cast<VkSurfaceKHR*>(l_renderingServer->GetVkSurface());

	if (vkCreateWin32SurfaceKHR(l_VkInstance, &l_createInfo, NULL, l_VkSurface) != VK_SUCCESS)
	{
		m_ObjectStatus = ObjectStatus::Created;
		InnoLogger::Log(LogLevel::Error, "WinVKWindowSurface: Failed to create window surface!");
		return false;
	}

	if (m_initConfig.engineMode == EngineMode::Host)
	{
		// Bring the window up on the screen and set it as main focus.
		ShowWindow(reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHwnd(), true);
		SetForegroundWindow(reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHwnd());
		SetFocus(reinterpret_cast<WinWindowSystem*>(g_pModuleManager->getWindowSystem())->getHwnd());
	}

	InnoLogger::Log(LogLevel::Success, "WinVKWindowSurface has been initialized.");
	return true;
}

bool WinVKWindowSurfaceNS::update()
{
	return true;
}

bool WinVKWindowSurfaceNS::terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "WinVKWindowSurfaceNS has been terminated.");

	return true;
}

bool WinVKWindowSurface::setup(void* hInstance, void* hwnd, void* WindowProc)
{
	return WinVKWindowSurfaceNS::setup(hInstance, hwnd, WindowProc);
}

bool WinVKWindowSurface::initialize()
{
	return WinVKWindowSurfaceNS::initialize();
}

bool WinVKWindowSurface::update()
{
	return WinVKWindowSurfaceNS::update();
}

bool WinVKWindowSurface::terminate()
{
	return WinVKWindowSurfaceNS::terminate();
}

ObjectStatus WinVKWindowSurface::getStatus()
{
	return WinVKWindowSurfaceNS::m_ObjectStatus;
}

bool WinVKWindowSurface::swapBuffer()
{
	return true;
}