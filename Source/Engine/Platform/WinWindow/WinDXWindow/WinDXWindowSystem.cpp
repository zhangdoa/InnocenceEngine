#include "WinDXWindowSystem.h"
#include "../../../Component/WinWindowSystemComponent.h"
#include "../../../Component/DX11RenderingBackendComponent.h"

#include "../../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE WinDXWindowSystemNS
{
	bool setup(void* hInstance, void* hwnd, void* WindowProc);
	bool initialize();
	bool update();
	bool terminate();

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
}

bool WinDXWindowSystemNS::setup(void* hInstance, void* hwnd, void* WindowProc)
{
	m_initConfig = g_pModuleManager->getInitConfig();

	// Setup the windows class with default settings.
	WNDCLASS wc = {};

	auto l_windowName = g_pModuleManager->getApplicationName();

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
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
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

	if (m_initConfig.engineMode == EngineMode::GAME)
	{
		// Bring the window up on the screen and set it as main focus.
		ShowWindow(WinWindowSystemComponent::get().m_hwnd, true);
		SetForegroundWindow(WinWindowSystemComponent::get().m_hwnd);
		SetFocus(WinWindowSystemComponent::get().m_hwnd);
	}

	m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinDXWindowSystem setup finished.");

	return true;
}

bool WinDXWindowSystemNS::initialize()
{
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinDXWindowSystem has been initialized.");
	return true;
}

bool WinDXWindowSystemNS::update()
{
	return true;
}

bool WinDXWindowSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WinGLWindowSystemNS has been terminated.");

	return true;
}

bool WinDXWindowSystem::setup(void* hInstance, void* hwnd, void* WindowProc)
{
	return WinDXWindowSystemNS::setup(hInstance, hwnd, WindowProc);
}

bool WinDXWindowSystem::initialize()
{
	return WinDXWindowSystemNS::initialize();
}

bool WinDXWindowSystem::update()
{
	return WinDXWindowSystemNS::update();
}

bool WinDXWindowSystem::terminate()
{
	return WinDXWindowSystemNS::terminate();
}

ObjectStatus WinDXWindowSystem::getStatus()
{
	return WinDXWindowSystemNS::m_objectStatus;
}

void WinDXWindowSystem::swapBuffer()
{
	if (WinDXWindowSystemNS::m_initConfig.renderingBackend == RenderingBackend::DX11)
	{
		auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

		// Present the back buffer to the screen since rendering is complete.
		if (l_renderingConfig.VSync)
		{
			// Lock to screen refresh rate.
			DX11RenderingBackendComponent::get().m_swapChain->Present(1, 0);
		}
		else
		{
			// Present as fast as possible.
			DX11RenderingBackendComponent::get().m_swapChain->Present(0, 0);
		}
	}
	else if (WinDXWindowSystemNS::m_initConfig.renderingBackend == RenderingBackend::DX12)
	{
	}
}