#include "VisionSystem.h"

#include "RenderingFrontendSystem.h"

#if defined INNO_PLATFORM_WIN
#include "DXRenderingBackend/DXWindowSystem.h"
#include "DXRenderingBackend/DXRenderingSystem.h"
#include "DXRenderingBackend/DXGuiSystem.h"
#endif

#include "GLRenderingBackend/GLWindowSystem.h"
#include "GLRenderingBackend/GLRenderingSystem.h"
#include "GLRenderingBackend/GLGuiSystem.h"

#if defined INNO_RENDERER_VULKAN
#include "VKRenderingBackend/VKWindowSystem.h"
#include "VKRenderingBackend/VKRenderingSystem.h"
#include "VKRenderingBackend/VKGuiSystem.h"
#endif

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

enum EngineMode { GAME, EDITOR };
enum RenderingBackend { GL, DX, VK };
struct initConfig
{
	EngineMode engineMode = EngineMode::GAME;
	RenderingBackend renderingBackend = RenderingBackend::GL;
};

INNO_PRIVATE_SCOPE InnoVisionSystemNS
{
	IWindowSystem* m_windowSystem;
	IRenderingFrontendSystem* m_renderingFrontendSystem;
	IRenderingBackendSystem* m_renderingBackendSystem;
	IGuiSystem* m_guiSystem;

	bool setupWindow(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);
	bool setupRendering();
	bool setupGui();

	std::vector<InnoFuture<void>> m_asyncTask;

	std::atomic<bool> m_isRendering = false;
	std::atomic<bool> m_allowRender = false;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	initConfig m_initConfig;

	initConfig getInitConfig(const std::string& arg);
}

initConfig InnoVisionSystemNS::getInitConfig(const std::string& arg)
{
	initConfig l_result;

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
			l_result.renderingBackend = RenderingBackend::GL;
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_PLATFORM_WIN
			l_result.renderingBackend = RenderingBackend::DX;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: DirectX is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.renderingBackend = RenderingBackend::VK;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: Vulkan is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VisionSystem: Unsupported rendering backend, use default OpenGL rendering backend.");
		}
	}

	return l_result;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	InnoVisionSystemNS::m_renderingFrontendSystem = new InnoRenderingFrontendSystem();

	std::string l_windowArguments = pScmdline;

	InnoVisionSystemNS::m_initConfig = InnoVisionSystemNS::getInitConfig(l_windowArguments);

	switch (InnoVisionSystemNS::m_initConfig.renderingBackend)
	{
	case RenderingBackend::GL:
		InnoVisionSystemNS::m_windowSystem = new GLWindowSystem();
		InnoVisionSystemNS::m_renderingBackendSystem = new GLRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new GLGuiSystem();
		break;
	case RenderingBackend::DX:
#if defined INNO_PLATFORM_WIN
		InnoVisionSystemNS::m_windowSystem = new DXWindowSystem();
		InnoVisionSystemNS::m_renderingBackendSystem = new DXRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new DXGuiSystem();
#endif
		break;
	case RenderingBackend::VK:
#if defined INNO_RENDERER_VULKAN
		InnoVisionSystemNS::m_windowSystem = new VKWindowSystem();
		InnoVisionSystemNS::m_renderingBackendSystem = new VKRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new VKGuiSystem();
#endif
		break;
	default:
		break;
	}

	if (InnoVisionSystemNS::m_initConfig.engineMode == EngineMode::GAME)
	{
		if (!InnoVisionSystemNS::setupWindow(hInstance, hPrevInstance, pScmdline, nCmdshow))
		{
			InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
			return false;
		};
	}
	else
	{
		// Editor handle window surface
	}

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

bool InnoVisionSystemNS::setupWindow(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	if (!InnoVisionSystemNS::m_windowSystem->setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
	{
		return false;
	}
	return true;
}

bool InnoVisionSystemNS::setupRendering()
{
	if (!InnoVisionSystemNS::m_renderingBackendSystem->setup(m_renderingFrontendSystem))
	{
		return false;
	}

	return true;
}

bool InnoVisionSystemNS::setupGui()
{
	if (!InnoVisionSystemNS::m_guiSystem->setup())
	{
		return false;
	}
	return true;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::initialize()
{
	if (InnoVisionSystemNS::m_initConfig.engineMode == EngineMode::GAME)
	{
		InnoVisionSystemNS::m_windowSystem->initialize();
	}
	else
	{
		// Editor handle window surface
	}

	InnoVisionSystemNS::m_renderingFrontendSystem->initialize();
	InnoVisionSystemNS::m_renderingBackendSystem->initialize();
	InnoVisionSystemNS::m_guiSystem->initialize();

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

	auto prepareRenderDataTask = g_pCoreSystem->getTaskSystem()->submit([]()
	{
	});

	InnoVisionSystemNS::m_asyncTask.emplace_back(std::move(prepareRenderDataTask));

	g_pCoreSystem->getTaskSystem()->shrinkFutureContainer(InnoVisionSystemNS::m_asyncTask);

	if (!InnoVisionSystemNS::m_allowRender)
	{
		InnoVisionSystemNS::m_renderingFrontendSystem->update();

		InnoVisionSystemNS::m_allowRender = true;
	}

	if (InnoVisionSystemNS::m_windowSystem->getStatus() == ObjectStatus::ALIVE)
	{
		if (InnoVisionSystemNS::m_initConfig.engineMode == EngineMode::GAME)
		{
			InnoVisionSystemNS::m_windowSystem->update();
		}
		else
		{
			// Editor handle window surface
		}

		if (!InnoVisionSystemNS::m_isRendering && InnoVisionSystemNS::m_allowRender)
		{
			InnoVisionSystemNS::m_allowRender = false;

			InnoVisionSystemNS::m_isRendering = true;

			InnoVisionSystemNS::m_renderingBackendSystem->update();
			InnoVisionSystemNS::m_guiSystem->update();
			if (InnoVisionSystemNS::m_initConfig.engineMode == EngineMode::GAME)
			{
				InnoVisionSystemNS::m_windowSystem->swapBuffer();
			}
			else
			{
				// Editor handle window surface
			}

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
	if (!InnoVisionSystemNS::m_guiSystem->terminate())
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
	if (InnoVisionSystemNS::m_initConfig.engineMode == EngineMode::GAME)
	{
		if (!InnoVisionSystemNS::m_windowSystem->terminate())
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "WindowSystem can't be terminated!");
			return false;
		}
	}
	else
	{
		// Editor handle window surface
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