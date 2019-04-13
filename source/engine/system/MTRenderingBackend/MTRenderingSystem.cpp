#include "MTRenderingSystem.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE MTRenderingSystemNS
{
	bool setup(IRenderingFrontendSystem* renderingFrontend);
  bool initialize();

  bool initializeDefaultAssets();

  bool update();
  bool terminate();

	EntityID m_entityID;

  ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
  IRenderingFrontendSystem* m_renderingFrontendSystem;
}

bool MTRenderingSystemNS::setup(IRenderingFrontendSystem* renderingFrontend)
{
	m_renderingFrontendSystem = renderingFrontend;
	m_entityID = InnoMath::createEntityID();

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingSystem setup finished.");
	return true;
}

bool MTRenderingSystemNS::initialize()
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingSystem has been initialized.");
	return true;
}

bool MTRenderingSystemNS::update()
{
	return true;
}

bool MTRenderingSystemNS::terminate()
{

	MTRenderingSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingSystem has been terminated.");

	return true;
}

bool MTRenderingSystem::setup(IRenderingFrontendSystem* renderingFrontend)
{
	bool result = m_bridge->setup();
	return MTRenderingSystemNS::setup(renderingFrontend);
}

bool MTRenderingSystem::initialize()
{
	bool result = m_bridge->initialize();
	return MTRenderingSystemNS::initialize();
}

bool MTRenderingSystem::update()
{
	bool result = m_bridge->update();
	return MTRenderingSystemNS::update();
}

bool MTRenderingSystem::terminate()
{
	bool result = m_bridge->terminate();
	return MTRenderingSystemNS::terminate();
}

ObjectStatus MTRenderingSystem::getStatus()
{
	return MTRenderingSystemNS::m_objectStatus;
}

bool MTRenderingSystem::resize()
{
	return true;
}

bool MTRenderingSystem::reloadShader(RenderPassType renderPassType)
{
	return true;
}

bool MTRenderingSystem::bakeGI()
{
	return true;
}

void MTRenderingSystem::setBridge(const std::shared_ptr<MTRenderingSystemBridge>& bridge)
{
	m_bridge = bridge;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingSystem: Bridge connected at " + InnoUtility::pointerToString(bridge.get()));
}
