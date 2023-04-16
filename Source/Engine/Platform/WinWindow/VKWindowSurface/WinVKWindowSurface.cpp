#include "WinVKWindowSurface.h"
#include "../WinWindowSystem.h"
#include "../../../Core/InnoLogger.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "../../../RenderingServer/VK/VKRenderingServer.h"
#include "../../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

namespace WinVKWindowSurfaceNS
{
	bool Setup(ISystemConfig* systemConfig);
	bool Initialize();
	bool Update();
	bool Terminate();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
}

bool WinVKWindowSurfaceNS::Setup(ISystemConfig* systemConfig)
{
	auto l_windowSurfaceConfig = reinterpret_cast<IWindowSurfaceConfig*>(systemConfig);

	m_initConfig = g_Engine->getInitConfig();

	if (m_initConfig.engineMode == EngineMode::Host)
	{
		// Setup the windows class with default settings.
		auto l_windowName = g_Engine->getApplicationName();

		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wcex.lpfnWndProc = (WNDPROC)l_windowSurfaceConfig->WindowProc;
		wcex.hInstance = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHInstance();
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpszClassName = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getApplicationName();

		auto l_windowClass = MAKEINTATOM(RegisterClassEx(&wcex));

		// Determine the resolution of the clients desktop screen.
		auto l_screenResolution = g_Engine->getRenderingFrontend()->getScreenResolution();
		auto l_screenWidth = (int32_t)l_screenResolution.x;
		auto l_screenHeight = (int32_t)l_screenResolution.y;

		RECT l_rect;
		l_rect.right = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
		l_rect.bottom = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

		// create a new window and context
		auto l_hwnd = CreateWindow(
			l_windowClass, reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getApplicationName(), // class name, window name
			WS_OVERLAPPEDWINDOW, // styles
			l_rect.right, l_rect.bottom, // posx, posy. If x is set to CW_USEDEFAULT y is ignored
			l_screenWidth, l_screenHeight, // width, height
			NULL, NULL, // parent window, menu
			reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHInstance(), NULL); // instance, param

		reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->setHwnd(l_hwnd);
	}

	m_ObjectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "WinVKWindowSurface Setup finished.");

	return true;
}

bool WinVKWindowSurfaceNS::Initialize()
{
	VkWin32SurfaceCreateInfoKHR l_createInfo = {};
	l_createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	l_createInfo.pNext = NULL;
	l_createInfo.hinstance = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHInstance();
	l_createInfo.hwnd = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHwnd();

	auto l_renderingServer = reinterpret_cast<VKRenderingServer*>(g_Engine->getRenderingServer());
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
		ShowWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHwnd(), true);
		SetForegroundWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHwnd());
		SetFocus(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHwnd());
	}

	InnoLogger::Log(LogLevel::Success, "WinVKWindowSurface has been initialized.");
	return true;
}

bool WinVKWindowSurfaceNS::Update()
{
	return true;
}

bool WinVKWindowSurfaceNS::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "WinVKWindowSurfaceNS has been terminated.");

	return true;
}

bool WinVKWindowSurface::Setup(ISystemConfig* systemConfig)
{
	return WinVKWindowSurfaceNS::Setup(systemConfig);
}

bool WinVKWindowSurface::Initialize()
{
	return WinVKWindowSurfaceNS::Initialize();
}

bool WinVKWindowSurface::Update()
{
	return WinVKWindowSurfaceNS::Update();
}

bool WinVKWindowSurface::Terminate()
{
	return WinVKWindowSurfaceNS::Terminate();
}

ObjectStatus WinVKWindowSurface::GetStatus()
{
	return WinVKWindowSurfaceNS::m_ObjectStatus;
}

bool WinVKWindowSurface::swapBuffer()
{
	return true;
}