#include "VisionSystem.h"

#include "../component/RenderingSystemSingletonComponent.h"
#include "../component/PhysicsSystemSingletonComponent.h"

#include "DXWindowSystem.h"
#include "DXRenderingSystem.h"
#include "DXGuiSystem.h"

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

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	std::string l_windowArguments = pScmdline;
	if (l_windowArguments == "")
	{
		g_pCoreSystem->getLogSystem()->printLog("Error: VisionSystem: No arguments found!");
		InnoVisionSystemNS::m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	auto l_argPos = l_windowArguments.find("renderer");
	if (l_argPos == 0)
	{
		g_pCoreSystem->getLogSystem()->printLog("Error: VisionSystem: No renderer argument found!");
		InnoVisionSystemNS::m_objectStatus = objectStatus::STANDBY;
		return false;
	}

	std::string l_rendererArguments = l_windowArguments.substr(l_argPos + 9);
	if (l_rendererArguments == "DX")
	{
		InnoVisionSystemNS::m_windowSystem = new DXWindowSystem();
		InnoVisionSystemNS::m_renderingSystem = new DXRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new DXGuiSystem();
	}
	else if (l_rendererArguments == "GL")
	{
		InnoVisionSystemNS::m_windowSystem = new GLWindowSystem();
		InnoVisionSystemNS::m_renderingSystem = new GLRenderingSystem();
		InnoVisionSystemNS::m_guiSystem = new GLGuiSystem();
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("Error::VisionSystem:: Incorrect renderer argument!");
		InnoVisionSystemNS::m_objectStatus = objectStatus::STANDBY;
		return false;
	}
	if (!InnoVisionSystemNS::setupWindow(hInstance, hPrevInstance, pScmdline, nCmdshow))
	{
		InnoVisionSystemNS::m_objectStatus = objectStatus::STANDBY;
		return false;
	};
	if (!InnoVisionSystemNS::setupRendering())
	{
		InnoVisionSystemNS::m_objectStatus = objectStatus::STANDBY;
		return false;
	}
	if (!InnoVisionSystemNS::setupGui())
	{
		InnoVisionSystemNS::m_objectStatus = objectStatus::STANDBY;
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
	RenderingSystemSingletonComponent::getInstance().m_canRender = true;

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

	InnoVisionSystemNS::m_objectStatus = objectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog("VisionSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::update()
{
	InnoVisionSystemNS::m_asyncTask = &g_pCoreSystem->getTaskSystem()->submit([]()
	{
		RenderingSystemSingletonComponent::getInstance().m_renderDataPack.clear();
		for (auto& i : PhysicsSystemSingletonComponent::getInstance().m_cullingDataPack)
		{
			renderDataPack l_renderDataPack;
			l_renderDataPack.m = i.m;
			l_renderDataPack.m_prev = i.m_prev;
			l_renderDataPack.normalMat = i.normalMat;
			auto l_visibleComponent = g_pCoreSystem->getGameSystem()->get<VisibleComponent>(i.visibleComponentEntityID);
			auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(i.MDCEntityID);
			auto l_modelPair = l_visibleComponent->m_modelMap.find(l_MDC);
			l_renderDataPack.MDC = l_MDC;
			l_renderDataPack.Material = l_modelPair->second;
			l_renderDataPack.visiblilityType = i.visiblilityType;
			RenderingSystemSingletonComponent::getInstance().m_renderDataPack.emplace_back(l_renderDataPack);
		};
	});

	InnoVisionSystemNS::m_windowSystem->update();

	if (InnoVisionSystemNS::m_windowSystem->getStatus() == objectStatus::ALIVE)
	{
		if (RenderingSystemSingletonComponent::getInstance().m_canRender)
		{
			RenderingSystemSingletonComponent::getInstance().m_canRender = false;
			InnoVisionSystemNS::m_renderingSystem->update();
			InnoVisionSystemNS::m_guiSystem->update();
			InnoVisionSystemNS::m_windowSystem->swapBuffer();
			RenderingSystemSingletonComponent::getInstance().m_canRender = true;
		}
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog("VisionSystem is stand-by.");
		InnoVisionSystemNS::m_objectStatus = objectStatus::STANDBY;
		return false;
	}
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::terminate()
{
	if (!InnoVisionSystemNS::m_guiSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog("GuiSystem can't be terminated!");
		return false;
	}
	if (!InnoVisionSystemNS::m_renderingSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog("RenderingSystem can't be terminated!");
		return false;
	}
	if (!InnoVisionSystemNS::m_windowSystem->terminate())
	{
		g_pCoreSystem->getLogSystem()->printLog("WindowSystem can't be terminated!");
		return false;
	}
	InnoVisionSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog("VisionSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT objectStatus InnoVisionSystem::getStatus()
{
	return InnoVisionSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT bool InnoVisionSystem::resize()
{
	InnoVisionSystemNS::m_renderingSystem->resize();
	return true;
}
