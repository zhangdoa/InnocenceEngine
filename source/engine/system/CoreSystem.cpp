#include "CoreSystem.h"
#include "Core/TimeSystem.h"
#include "Core/LogSystem.h"
#include "Core/MemorySystem.h"
#include "Core/TaskSystem.h"
#include "TestSystem.h"
#include "FileSystem/FileSystem.h"
#include "GameSystem.h"
#include "AssetSystem.h"
#include "PhysicsSystem.h"
#include "InputSystem.h"
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
#if defined INNO_PLATFORM_LINUX
#include "LinuxWindow/LinuxWindowSystem.h"
#endif
#if defined INNO_RENDERER_VULKAN
#include "VKRenderingBackend/VKRenderingSystem.h"
#endif

#include "ImGuiWrapper/ImGuiWrapper.h"

ICoreSystem* g_pCoreSystem;

#define createSubSystemInstanceDefi( className ) \
m_##className = std::make_unique<Inno##className>(); \
if (!m_##className.get()) \
{ \
	return false; \
} \

#define subSystemSetup( className ) \
if (!g_pCoreSystem->get##className()->setup()) \
{ \
	return false; \
} \
g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, std::string(#className) + " setup finished."); \

#define subSystemInit( className ) \
if (!g_pCoreSystem->get##className()->initialize()) \
{ \
	return false; \
} \

#define subSystemUpdate( className ) \
if (!g_pCoreSystem->get##className()->update()) \
{ \
m_objectStatus = ObjectStatus::Suspended; \
return false; \
}

#define subSystemTerm( className ) \
if (!g_pCoreSystem->get##className()->terminate()) \
{ \
	return false; \
} \

#define subSystemGetDefi( className ) \
INNO_SYSTEM_EXPORT I##className * InnoCoreSystem::get##className() \
{ \
	return InnoCoreSystemNS::m_##className.get(); \
} \

INNO_PRIVATE_SCOPE InnoCoreSystemNS
{
	InitConfig parseInitConfig(const std::string& arg);
	bool createSubSystemInstance(char* pScmdline);
	bool setup(void* appHook, void* extraHook, char* pScmdline);
	bool initialize();
	bool update();
	bool terminate();

	InitConfig m_initConfig;

	std::unique_ptr<ITimeSystem> m_TimeSystem;
	std::unique_ptr<ILogSystem> m_LogSystem;
	std::unique_ptr<IMemorySystem> m_MemorySystem;
	std::unique_ptr<ITaskSystem> m_TaskSystem;
	std::unique_ptr<ITestSystem> m_TestSystem;
	std::unique_ptr<IFileSystem> m_FileSystem;
	std::unique_ptr<IGameSystem> m_GameSystem;
	std::unique_ptr<IAssetSystem> m_AssetSystem;
	std::unique_ptr<IPhysicsSystem> m_PhysicsSystem;
	std::unique_ptr<IInputSystem> m_InputSystem;
	std::unique_ptr<IWindowSystem> m_WindowSystem;
	std::unique_ptr<IRenderingFrontendSystem> m_RenderingFrontendSystem;
	std::unique_ptr<IRenderingBackendSystem> m_RenderingBackendSystem;

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	bool m_showImGui = false;
	std::function<void()> f_toggleshowImGui;

	std::atomic<bool> m_isRendering = false;
	std::atomic<bool> m_allowRender = false;
}

InitConfig InnoCoreSystemNS::parseInitConfig(const std::string& arg)
{
	InitConfig l_result;

	if (arg == "")
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: No arguments found, use default settings.");
		return l_result;
	}

	auto l_engineModeArgPos = arg.find("mode");

	if (l_engineModeArgPos == std::string::npos)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: No engine mode argument found, use default game mode.");
	}
	else
	{
		std::string l_engineModeArguments = arg.substr(l_engineModeArgPos + 5);
		l_engineModeArguments = l_engineModeArguments.substr(0, 1);

		if (l_engineModeArguments == "0")
		{
			l_result.engineMode = EngineMode::GAME;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "CoreSystem: Launch in game mode.");
		}
		else if (l_engineModeArguments == "1")
		{
			l_result.engineMode = EngineMode::EDITOR;
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "CoreSystem: Launch in editor mode.");
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: Unsupported engine mode.");
		}
	}

	auto l_renderingBackendArgPos = arg.find("renderer");

	if (l_renderingBackendArgPos == std::string::npos)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: No rendering backend argument found, use default OpenGL rendering backend.");
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
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: OpenGL is not supported on current platform, no rendering backend will be launched.");
#endif
		}
		else if (l_rendererArguments == "1")
		{
#if defined INNO_PLATFORM_WIN
			l_result.renderingBackend = RenderingBackend::DX11;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: DirectX 11 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "2")
		{
#if defined INNO_PLATFORM_WIN
			l_result.renderingBackend = RenderingBackend::DX12;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: DirectX 12 is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "3")
		{
#if defined INNO_RENDERER_VULKAN
			l_result.renderingBackend = RenderingBackend::VK;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: Vulkan is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else if (l_rendererArguments == "4")
		{
#if defined INNO_PLATFORM_MAC
			l_result.renderingBackend = RenderingBackend::MT;
#else
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: Metal is not supported on current platform, use default OpenGL rendering backend.");
#endif
		}
		else
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "CoreSystem: Unsupported rendering backend, use default OpenGL rendering backend.");
		}
	}

	return l_result;
}

bool InnoCoreSystemNS::createSubSystemInstance(char* pScmdline)
{
	createSubSystemInstanceDefi(TimeSystem);
	createSubSystemInstanceDefi(LogSystem);
	createSubSystemInstanceDefi(MemorySystem);
	createSubSystemInstanceDefi(TaskSystem);

	createSubSystemInstanceDefi(TestSystem);
	createSubSystemInstanceDefi(FileSystem);
	createSubSystemInstanceDefi(GameSystem);
	createSubSystemInstanceDefi(AssetSystem);
	createSubSystemInstanceDefi(PhysicsSystem);
	createSubSystemInstanceDefi(InputSystem);

	std::string l_windowArguments = pScmdline;
	m_initConfig = parseInitConfig(l_windowArguments);

#if defined INNO_PLATFORM_WIN
	m_WindowSystem = std::make_unique<WinWindowSystem>();
	if (!m_WindowSystem.get())
	{
		return false;
	}
#endif
#if defined INNO_PLATFORM_MAC
	m_WindowSystem = std::make_unique<MacWindowSystem>();
	if (!m_WindowSystem.get())
	{
		return false;
	}
#endif
#if defined INNO_PLATFORM_LINUX
	m_WindowSystem = std::make_unique<LinuxWindowSystem>();
	if (!m_WindowSystem.get())
	{
		return false;
	}
#endif

	m_RenderingFrontendSystem = std::make_unique<InnoRenderingFrontendSystem>();
	if (!m_RenderingFrontendSystem.get())
	{
		return false;
	}

	switch (m_initConfig.renderingBackend)
	{
	case RenderingBackend::GL:
#if !defined INNO_PLATFORM_MAC && defined INNO_RENDERER_OPENGL
		m_RenderingBackendSystem = std::make_unique<GLRenderingSystem>();
		if (!m_RenderingBackendSystem.get())
		{
			return false;
		}
#endif
		break;
	case RenderingBackend::DX11:
#if defined INNO_PLATFORM_WIN
		m_RenderingBackendSystem = std::make_unique<DX11RenderingSystem>();
		if (!m_RenderingBackendSystem.get())
		{
			return false;
		}
#endif
		break;
	case RenderingBackend::DX12:
#if defined INNO_PLATFORM_WIN
		m_RenderingBackendSystem = std::make_unique<DX12RenderingSystem>();
		if (!m_RenderingBackendSystem.get())
		{
			return false;
		}
#endif
		break;
	case RenderingBackend::VK:
#if defined INNO_RENDERER_VULKAN
		m_RenderingBackendSystem = std::make_unique<VKRenderingSystem>();
		if (!m_RenderingBackendSystem.get())
		{
			return false;
		}
#endif
		break;
	case RenderingBackend::MT:
#if defined INNO_PLATFORM_MAC
		m_RenderingBackendSystem = std::make_unique<MTRenderingSystem>();
		if (!m_RenderingBackendSystem.get())
		{
			return false;
		}
#endif
		break;
	default:
		break;
	}

	// Objective-C++ bridge class instances passed as the 1st and 2nd parameters of setup()
#if defined INNO_PLATFORM_MAC
	auto l_windowSystem = reinterpret_cast<MacWindowSystem*>(m_WindowSystem.get());
	auto l_windowSystemBridge = reinterpret_cast<MacWindowSystemBridge*>(appHook);

	l_windowSystem->setBridge(l_windowSystemBridge);

	auto l_renderingBackendSystem = reinterpret_cast<MTRenderingSystem*>(m_RenderingBackendSystem.get());
	auto l_renderingBackendSystemBridge = reinterpret_cast<MTRenderingSystemBridge*>(extraHook);

	l_renderingBackendSystem->setBridge(l_renderingBackendSystemBridge);
#endif

	return true;
}

bool InnoCoreSystemNS::setup(void* appHook, void* extraHook, char* pScmdline)
{
	if (!InnoCoreSystemNS::createSubSystemInstance(pScmdline))
	{
		return false;
	}

	subSystemSetup(TimeSystem);
	subSystemSetup(LogSystem);
	subSystemSetup(MemorySystem);
	subSystemSetup(TaskSystem);

	subSystemSetup(TestSystem);

	f_toggleshowImGui = [&]() {
		m_showImGui = !m_showImGui;
	};
	g_pCoreSystem->getInputSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_I, ButtonStatus::PRESSED }, &f_toggleshowImGui);

	if (!m_WindowSystem->setup(appHook, extraHook))
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "WindowSystem setup finished.");

	if (!m_RenderingBackendSystem->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingBackendSystem setup finished.");

	if (!m_RenderingFrontendSystem->setup(m_RenderingBackendSystem.get()))
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "RenderingFrontendSystem setup finished.");

	if (!ImGuiWrapper::get().setup())
	{
		return false;
	}

	subSystemSetup(AssetSystem);
	subSystemSetup(FileSystem);
	subSystemSetup(GameSystem);
	subSystemSetup(PhysicsSystem);
	subSystemSetup(InputSystem);

	m_objectStatus = ObjectStatus::Created;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine setup finished.");
	return true;
}

bool InnoCoreSystemNS::initialize()
{
	subSystemInit(TimeSystem);
	subSystemInit(LogSystem);
	subSystemInit(MemorySystem);
	subSystemInit(TaskSystem);

	subSystemInit(TestSystem);

	subSystemInit(FileSystem);
	subSystemInit(GameSystem);
	subSystemInit(AssetSystem);
	subSystemInit(PhysicsSystem);
	subSystemInit(InputSystem);

	m_WindowSystem->initialize();
	m_RenderingBackendSystem->initialize();
	m_RenderingFrontendSystem->initialize();
	ImGuiWrapper::get().initialize();

	m_objectStatus = ObjectStatus::Activated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been initialized.");
	return true;
}

bool InnoCoreSystemNS::update()
{
	subSystemUpdate(TimeSystem);
	subSystemUpdate(LogSystem);
	subSystemUpdate(MemorySystem);
	subSystemUpdate(TaskSystem);

	subSystemUpdate(TestSystem);

	subSystemUpdate(FileSystem);
	subSystemUpdate(GameSystem);
	subSystemUpdate(AssetSystem);
	subSystemUpdate(PhysicsSystem);
	subSystemUpdate(InputSystem);

	if (m_WindowSystem->getStatus() == ObjectStatus::Activated)
	{
		if (g_pCoreSystem->getFileSystem()->isLoadingScene())
		{
			return true;
		}

		m_WindowSystem->update();

		if (!m_allowRender)
		{
			m_RenderingFrontendSystem->update();

			m_allowRender = true;
		}

		if (!m_isRendering && m_allowRender)
		{
			m_allowRender = false;

			m_isRendering = true;

			m_RenderingBackendSystem->update();

			m_RenderingBackendSystem->render();

			if (m_showImGui)
			{
				ImGuiWrapper::get().update();
			}

			m_WindowSystem->swapBuffer();

			m_isRendering = false;
		}

		g_pCoreSystem->getGameSystem()->saveComponentsCapture();

		return true;
	}
	else
	{
		m_objectStatus = ObjectStatus::Suspended;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "Engine is stand-by.");
		return false;
	}
	return true;
}

bool InnoCoreSystemNS::terminate()
{
	if (!ImGuiWrapper::get().terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GuiSystem can't be terminated!");
		return false;
	}

	if (!m_RenderingBackendSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem can't be terminated!");
		return false;
	}
	if (!m_RenderingFrontendSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingFrontendSystem can't be terminated!");
		return false;
	}

	if (!m_WindowSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "WindowSystem can't be terminated!");
		return false;
	}

	subSystemTerm(InputSystem);
	subSystemTerm(PhysicsSystem);
	subSystemTerm(AssetSystem);
	subSystemTerm(GameSystem);
	subSystemTerm(FileSystem);

	subSystemTerm(TestSystem);

	subSystemTerm(TaskSystem);
	subSystemTerm(MemorySystem);
	subSystemTerm(LogSystem);
	subSystemTerm(TimeSystem);

	m_objectStatus = ObjectStatus::Terminated;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been terminated.");

	return true;
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::setup(void* appHook, void* extraHook, char* pScmdline)
{
	g_pCoreSystem = this;

	return InnoCoreSystemNS::setup(appHook, extraHook, pScmdline);
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::initialize()
{
	return InnoCoreSystemNS::initialize();
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::update()
{
	return InnoCoreSystemNS::update();
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::terminate()
{
	return InnoCoreSystemNS::terminate();
}

INNO_SYSTEM_EXPORT ObjectStatus InnoCoreSystem::getStatus()
{
	return InnoCoreSystemNS::m_objectStatus;
}

subSystemGetDefi(TimeSystem);
subSystemGetDefi(LogSystem);
subSystemGetDefi(MemorySystem);
subSystemGetDefi(TaskSystem);

subSystemGetDefi(TestSystem);

subSystemGetDefi(FileSystem);
subSystemGetDefi(GameSystem);
subSystemGetDefi(AssetSystem);
subSystemGetDefi(PhysicsSystem);
subSystemGetDefi(InputSystem);

INNO_SYSTEM_EXPORT IWindowSystem * InnoCoreSystem::getWindowSystem()
{
	return InnoCoreSystemNS::m_WindowSystem.get();
}

INNO_SYSTEM_EXPORT IRenderingFrontendSystem * InnoCoreSystem::getRenderingFrontendSystem()
{
	return InnoCoreSystemNS::m_RenderingFrontendSystem.get();
}

INNO_SYSTEM_EXPORT IRenderingBackendSystem * InnoCoreSystem::getRenderingBackendSystem()
{
	return InnoCoreSystemNS::m_RenderingBackendSystem.get();
}

INNO_SYSTEM_EXPORT InitConfig InnoCoreSystem::getInitConfig()
{
	return InnoCoreSystemNS::m_initConfig;
}