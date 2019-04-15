#include "VisionSystem.h"

#include "RenderingFrontendSystem.h"

#if defined INNO_PLATFORM_WIN
#include "WinWindow/WinWindowSystem.h"
#include "DX11RenderingBackend/DX11RenderingSystem.h"
#include "DX12RenderingBackend/DX12RenderingSystem.h"
#endif

#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
#include "GLRenderingBackend/GLRenderingSystem.h"
#endif

#if defined INNO_PLATFORM_MAC
#include "MacWindow/MacWindowSystem.h"
#include "MTRenderingBackend/MTRenderingSystem.h"
#endif

#if defined INNO_RENDERER_VULKAN
#include "VKRenderingBackend/VKRenderingSystem.h"
#endif

#include "ImGuiWrapper/ImGuiWrapper.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoVisionSystemNS
{
	IWindowSystem* m_windowSystem;
	IRenderingFrontendSystem* m_renderingFrontendSystem;
	IRenderingBackendSystem* m_renderingBackendSystem;

	bool setupWindow(void* hInstance, void* hwnd);
	bool setupRendering();
	bool setupGui();

	std::atomic<bool> m_isRendering = false;
	std::atomic<bool> m_allowRender = false;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	InitConfig m_initConfig;

	InitConfig parseInitConfig(const std::string& arg);
}

InitConfig InnoVisionSystemNS::parseInitConfig(const std::string& arg)
{
	InitConfig l_result;

	if (arg == "")
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: No arguments found, use default settings.");
		return l_result;
	}

	auto l_engineModeArgPos = arg.find("mode");

	if (l_engineModeArgPos == std::string::npos)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: No engine mode argument found, use default game mode.");
	}
	else
	{
		std::string l_engineModeArguments = arg.substr(l_engineModeArgPos + 5);
		l_engineModeArguments = l_engineModeArguments.substr(0, 1);

		if (l_engineModeArguments == "0")
		{
			l_result.engineMode = EngineMode::GAME;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VisionSystem: Launch in game mode.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::EDITOR;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "VisionSystem: Launch in editor mode.");
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: Unsupported engine mode.");
		}
	}

	auto l_renderingBackendArgPos = arg.find("renderer");

	if (l_renderingBackendArgPos == std::string::npos)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: No rendering backend argument found, use default OpenGL rendering backend.");
	}
	else
	{
		std::string l_rendererArguments = arg.substr(l_renderingBackendArgPos + 9);
		l_rendererArguments = l_rendererArguments.substr(0, 1);

		if (l_rendererArguments == "0")
		{
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
			l_result.renderingBackend = RenderingBackend::GL;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: OpenGL is not supported on current platform, no rendering backend will be launched.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_PLATFORM_WIN
			l_result.renderingBackend = RenderingBackend::DX11;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: DirectX 11 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_PLATFORM_WIN
			l_result.renderingBackend = RenderingBackend::DX12;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: DirectX 12 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "3")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.renderingBackend = RenderingBackend::VK;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: Vulkan is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "4")
		{
#if defined INNO_PLATFORM_MAC
			l_result.renderingBackend = RenderingBackend::MT;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: Metal is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: Unsupported rendering backend, use default OpenGL rendering backend.");
		}
	}

	return l_result;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::setup(void* appHook, void* extraHook, char* pScmdline)
{
	InnoVisionSystemNS::m_renderingFrontendSystem = new InnoRenderingFrontendSystem();

	std::string l_windowArguments = pScmdline;

	InnoVisionSystemNS::m_initConfig = InnoVisionSystemNS::parseInitConfig(l_windowArguments);

#if defined INNO_PLATFORM_WIN
	InnoVisionSystemNS::m_windowSystem = new WinWindowSystem();
#endif

#if defined INNO_PLATFORM_MAC
	InnoVisionSystemNS::m_windowSystem = new MacWindowSystem();
#endif

	switch (InnoVisionSystemNS::m_initConfig.renderingBackend)
	{
	case RenderingBackend::GL:
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
		InnoVisionSystemNS::m_renderingBackendSystem = new GLRenderingSystem();
#endif
		break;
	case RenderingBackend::DX11:
#if defined INNO_PLATFORM_WIN
		InnoVisionSystemNS::m_renderingBackendSystem = new DX11RenderingSystem();
#endif
		break;
	case RenderingBackend::DX12:
#if defined INNO_PLATFORM_WIN
		InnoVisionSystemNS::m_renderingBackendSystem = new DX12RenderingSystem();
#endif
		break;
	case RenderingBackend::VK:
#if defined INNO_RENDERER_VULKAN
		InnoVisionSystemNS::m_renderingBackendSystem = new VKRenderingSystem();
#endif
		break;
	case RenderingBackend::MT:
#if defined INNO_PLATFORM_MAC
		InnoVisionSystemNS::m_renderingBackendSystem = new MTRenderingSystem();
#endif
		break;
	default:
		break;
	}

	// Objective-C++ bridge class instances passed as the 1st and 2nd parameters of setup()
#if defined INNO_PLATFORM_MAC
	auto l_windowSystem = reinterpret_cast<MacWindowSystem*>(InnoVisionSystemNS::m_windowSystem);
	auto l_windowSystemBridge = reinterpret_cast<MacWindowSystemBridge*>(appHook);

	l_windowSystem->setBridge(l_windowSystemBridge);

	auto l_renderingBackendSystem = reinterpret_cast<MTRenderingSystem*>(InnoVisionSystemNS::m_renderingBackendSystem);
	auto l_renderingBackendSystemBridge = reinterpret_cast<MTRenderingSystemBridge*>(extraHook);

	l_renderingBackendSystem->setBridge(l_renderingBackendSystemBridge);
#endif

	if (!InnoVisionSystemNS::setupWindow(appHook, extraHook))
	{
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	};

	if (!InnoVisionSystemNS::setupRendering())
	{
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}
	if (!InnoVisionSystemNS::setupGui())
	{
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	return true;
}

bool InnoVisionSystemNS::setupWindow(void* hInstance, void* hwnd)
{
	if (!InnoVisionSystemNS::m_windowSystem->setup(hInstance, hwnd))
	{
		return false;
	}
	return true;
}

bool InnoVisionSystemNS::setupRendering()
{
	if (!InnoVisionSystemNS::m_renderingFrontendSystem->setup())
	{
		return false;
	}
	if (!InnoVisionSystemNS::m_renderingBackendSystem->setup(m_renderingFrontendSystem))
	{
		return false;
	}

	return true;
}

bool InnoVisionSystemNS::setupGui()
{
	if (!ImGuiWrapper::get().setup())
	{
		return false;
	}

	return true;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::initialize()
{
	InnoVisionSystemNS::m_windowSystem->initialize();

	InnoVisionSystemNS::m_renderingFrontendSystem->initialize();
	InnoVisionSystemNS::m_renderingBackendSystem->initialize();
	ImGuiWrapper::get().initialize();

	InnoVisionSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VisionSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::update()
{
	if (g_pCoreSystem->getFileSystem()->isLoadingScene())
	{
		return true;
	}

	if (!InnoVisionSystemNS::m_allowRender)
	{
		InnoVisionSystemNS::m_renderingFrontendSystem->update();

		InnoVisionSystemNS::m_allowRender = true;
	}

	if (InnoVisionSystemNS::m_windowSystem->getStatus() == ObjectStatus::ALIVE)
	{
		InnoVisionSystemNS::m_windowSystem->update();

		if (!InnoVisionSystemNS::m_isRendering && InnoVisionSystemNS::m_allowRender)
		{
			InnoVisionSystemNS::m_allowRender = false;

			InnoVisionSystemNS::m_isRendering = true;

			InnoVisionSystemNS::m_renderingBackendSystem->update();

			//ImGuiWrapper::get().update();

			InnoVisionSystemNS::m_windowSystem->swapBuffer();

			InnoVisionSystemNS::m_isRendering = false;
		}
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem is stand-by.");
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::terminate()
{
	if (!ImGuiWrapper::get().terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GuiSystem can't be terminated!");
		return false;
	}

	if (!InnoVisionSystemNS::m_renderingBackendSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem can't be terminated!");
		return false;
	}
	if (!InnoVisionSystemNS::m_renderingFrontendSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontendSystem can't be terminated!");
		return false;
	}

	if (!InnoVisionSystemNS::m_windowSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "WindowSystem can't be terminated!");
		return false;
	}

	InnoVisionSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VisionSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT ObjectStatus InnoVisionSystem::getStatus()
{
	return InnoVisionSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT IWindowSystem * InnoVisionSystem::getWindowSystem()
{
	return InnoVisionSystemNS::m_windowSystem;
}

INNO_SYSTEM_EXPORT IRenderingFrontendSystem * InnoVisionSystem::getRenderingFrontend()
{
	return InnoVisionSystemNS::m_renderingFrontendSystem;
}

INNO_SYSTEM_EXPORT IRenderingBackendSystem * InnoVisionSystem::getRenderingBackend()
{
	return InnoVisionSystemNS::m_renderingBackendSystem;
}

INNO_SYSTEM_EXPORT InitConfig InnoVisionSystem::getInitConfig()
{
	return InnoVisionSystemNS::m_initConfig;
}