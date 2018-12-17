#include "VisionSystem.h"

#include "../component/RenderingSystemComponent.h"
#include "../component/PhysicsSystemComponent.h"

#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
#include "DXWindowSystem.h"
#include "DXRenderingSystem.h"
#include "DXGuiSystem.h"
#endif

#include "GLWindowSystem.h"
#include "GLRenderingSystem.h"
#include "GLGuiSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoVisionSystemNS
{
	IWindowSystem* m_windowSystem;
	IRenderingSystem* m_renderingSystem;
	IGuiSystem* m_guiSystem;

	bool setupWindow(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);
	bool setupRendering();
	bool setupGui();

	InnoFuture<void>* m_asyncTask;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	std::string l_windowArguments = pScmdline;
	if (l_windowArguments == "")
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: No arguments found!");
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	auto l_argPos = l_windowArguments.find("renderer");
	if (l_argPos == 0)
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: No renderer argument found!");
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}

	std::string l_rendererArguments = l_windowArguments.substr(l_argPos + 9);
	if (l_rendererArguments == "DX")
	{
		#if defined INNO_PLATFORM_WIN64 || defined INNO_PLATFORM_WIN32
		InnoVisionSystemNS::m_windowSystem = new DXWindowSystem();
		InnoVisionSystemNS::m_renderingSystem = new DXRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new DXGuiSystem();
		#else
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: DirectX is only supported on Windows OS!");
		return false;
		#endif
	}
	else if (l_rendererArguments == "GL")
	{
		InnoVisionSystemNS::m_windowSystem = new GLWindowSystem();
		InnoVisionSystemNS::m_renderingSystem = new GLRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new GLGuiSystem();
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VisionSystem: Incorrect renderer argument " + l_rendererArguments + " !");
		InnoVisionSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		return false;
	}
	if (!InnoVisionSystemNS::setupWindow(hInstance, hPrevInstance, pScmdline, nCmdshow))
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
	if (!InnoVisionSystemNS::m_renderingSystem->setup())
	{
		return false;
	}
	RenderingSystemComponent::get().m_canRender = true;

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
	InnoVisionSystemNS::m_windowSystem->initialize();
	InnoVisionSystemNS::m_renderingSystem->initialize();
	InnoVisionSystemNS::m_guiSystem->initialize();

	InnoVisionSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VisionSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::update()
{
	auto temp = g_pCoreSystem->getTaskSystem()->submit([]()
	{
	});
	InnoVisionSystemNS::m_asyncTask = &temp;

	RenderingSystemComponent::get().m_renderDataPack.clear();
	for (auto& i : PhysicsSystemComponent::get().m_cullingDataPack)
	{
		auto l_visibleComponent = g_pCoreSystem->getGameSystem()->get<VisibleComponent>(i.visibleComponentEntityID);
		if (l_visibleComponent != nullptr)
		{
			auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(i.MDCEntityID);
			if (l_MDC != nullptr)
			{
				auto l_modelPair = l_visibleComponent->m_modelMap.find(l_MDC);
				if (l_modelPair != l_visibleComponent->m_modelMap.end())
				{
					RenderDataPack l_renderDataPack;
					l_renderDataPack.m = i.m;
					l_renderDataPack.m_prev = i.m_prev;
					l_renderDataPack.normalMat = i.normalMat;
					l_renderDataPack.MDC = l_MDC;
					l_renderDataPack.Material = l_modelPair->second;
					l_renderDataPack.visiblilityType = i.visiblilityType;
					RenderingSystemComponent::get().m_renderDataPack.emplace_back(l_renderDataPack);
				}
			}
		}
	};

	InnoVisionSystemNS::m_windowSystem->update();

	if (InnoVisionSystemNS::m_windowSystem->getStatus() == ObjectStatus::ALIVE)
	{
		if (RenderingSystemComponent::get().m_canRender)
		{
			RenderingSystemComponent::get().m_canRender = false;
			InnoVisionSystemNS::m_renderingSystem->update();
			InnoVisionSystemNS::m_guiSystem->update();
			InnoVisionSystemNS::m_windowSystem->swapBuffer();
			RenderingSystemComponent::get().m_canRender = true;
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
	if (!InnoVisionSystemNS::m_renderingSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingSystem can't be terminated!");
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

INNO_SYSTEM_EXPORT bool InnoVisionSystem::resize()
{
	InnoVisionSystemNS::m_renderingSystem->resize();
	return true;
}
