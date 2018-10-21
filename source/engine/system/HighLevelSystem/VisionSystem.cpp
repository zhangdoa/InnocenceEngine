#include "VisionSystem.h"

#include "../LowLevelSystem/LogSystem.h"

#include "../../component/RenderingSystemSingletonComponent.h"

#include "../HighLevelSystem/GameSystem.h"

#include "../HighLevelSystem/DXWindowSystem.h"
#include "../HighLevelSystem/DXRenderingSystem.h"
#include "../HighLevelSystem/DXGuiSystem.h"

#include "../HighLevelSystem/GLWindowSystem.h"
#include "../HighLevelSystem/GLRenderingSystem.h"
#include "../HighLevelSystem/GLGuiSystem.h"

namespace InnoVisionSystem
{
	IWindowSystem* m_windowSystem;
	IRenderingSystem* m_renderingSystem;
	IGuiSystem* m_guiSystem;

	bool setupWindow(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);
	bool setupRendering();
	bool setupGui();

	objectStatus m_VisionSystemStatus = objectStatus::SHUTDOWN;
}

InnoHighLevelSystem_EXPORT bool InnoVisionSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	std::string l_windowArguments = pScmdline;
	if (l_windowArguments == "")
	{
		InnoLogSystem::printLog("Error: VisionSystem: No arguments found!");
		m_VisionSystemStatus = objectStatus::STANDBY;
		return false;
	}

	auto l_argPos = l_windowArguments.find("renderer");
	if (l_argPos == 0)
	{
		InnoLogSystem::printLog("Error: VisionSystem: No renderer argument found!");
		m_VisionSystemStatus = objectStatus::STANDBY;
		return false;
	}

	std::string l_rendererArguments = l_windowArguments.substr(l_argPos + 9);
	if (l_rendererArguments == "DX")
	{
		m_windowSystem = new DXWindowSystem();
		m_renderingSystem = new DXRenderingSystem();
		m_guiSystem = new DXGuiSystem();
	}
	else if (l_rendererArguments == "GL")
	{
		m_windowSystem = new GLWindowSystem();
		m_renderingSystem = new GLRenderingSystem();
		m_guiSystem = new GLGuiSystem();
	}
	else
	{
		InnoLogSystem::printLog("Error::VisionSystem:: Incorrect renderer argument!");
		m_VisionSystemStatus = objectStatus::STANDBY;
		return false;
	}
	if (!setupWindow(hInstance, hPrevInstance, pScmdline, nCmdshow))
	{
		m_VisionSystemStatus = objectStatus::STANDBY;
		return false;
	};
	if (!setupRendering())
	{
		m_VisionSystemStatus = objectStatus::STANDBY;
		return false;
	}
	if (!setupGui())
	{
		m_VisionSystemStatus = objectStatus::STANDBY;
		return false;
	}
	m_VisionSystemStatus = objectStatus::ALIVE;
	return true;
}

bool InnoVisionSystem::setupWindow(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	if (!m_windowSystem->setup(hInstance, hPrevInstance, pScmdline, nCmdshow))
	{
		return false;
	}
	return true;
}

bool InnoVisionSystem::setupRendering()
{
	if (!m_renderingSystem->setup())
	{
		return false;
	}
	RenderingSystemSingletonComponent::getInstance().m_canRender = true;

	return true;
}

bool InnoVisionSystem::setupGui()
{
	if (!m_guiSystem->setup())
	{
		return false;
	}
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoVisionSystem::initialize()
{
	m_windowSystem->initialize();
	m_renderingSystem->initialize();
	m_guiSystem->initialize();

	InnoLogSystem::printLog("VisionSystem has been initialized.");
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoVisionSystem::update()
{
	m_windowSystem->update();

	if (m_windowSystem->getStatus() == objectStatus::ALIVE)
	{
		if (RenderingSystemSingletonComponent::getInstance().m_canRender)
		{
			RenderingSystemSingletonComponent::getInstance().m_canRender = false;
			m_renderingSystem->update();
			m_guiSystem->update();
			m_windowSystem->swapBuffer();
			RenderingSystemSingletonComponent::getInstance().m_canRender = true;
		}
		return true;
	}
	else
	{
		InnoLogSystem::printLog("VisionSystem is stand-by.");
		m_VisionSystemStatus = objectStatus::STANDBY;
		return false;
	}
}

InnoHighLevelSystem_EXPORT bool InnoVisionSystem::terminate()
{
	if (m_windowSystem->getStatus() == objectStatus::ALIVE)
	{
		m_guiSystem->terminate();
		m_renderingSystem->terminate();
		m_windowSystem->terminate();	
		m_VisionSystemStatus = objectStatus::SHUTDOWN;
		InnoLogSystem::printLog("VisionSystem has been terminated.");
		return true;
	}
	else
	{
		return false;
	}
}

InnoHighLevelSystem_EXPORT objectStatus InnoVisionSystem::getStatus()
{
	return m_VisionSystemStatus;
}