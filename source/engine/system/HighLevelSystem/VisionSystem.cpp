#include "VisionSystem.h"

#include "../LowLevelSystem/LogSystem.h"

#include "../../component/WindowSystemSingletonComponent.h"
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
	using WindowSystem = DXWindowSystem::Instance;
	using RenderingSystem = DXRenderingSystem::Instance;
	using GuiSystem = DXGuiSystem::Instance;

	bool setupWindow();
	bool setupRendering();
	bool setupGui();

	objectStatus m_VisionSystemStatus = objectStatus::SHUTDOWN;
}

#if defined(INNO_RENDERER_DX)
InnoHighLevelSystem_EXPORT bool InnoVisionSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	DXWindowSystemSingletonComponent::getInstance().m_hInstance = static_cast<HINSTANCE>(hInstance);
	DXWindowSystemSingletonComponent::getInstance().m_pScmdline = pScmdline;
	DXWindowSystemSingletonComponent::getInstance().m_nCmdshow = nCmdshow;

	std::string l_windowArguments = DXWindowSystemSingletonComponent::getInstance().m_pScmdline;
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
		using WindowSystem = DXWindowSystem::Instance;
		using RenderingSystem = DXRenderingSystem::Instance;
		using GuiSystem = DXGuiSystem::Instance;
	}
	else if (l_rendererArguments == "GL")
	{
		using WindowSystem = GLWindowSystem::Instance;
		using RenderingSystem = GLRenderingSystem::Instance;
		using GuiSystem = GLGuiSystem::Instance;
	}
	else
	{
		InnoLogSystem::printLog("ERROR::VisionSystem:: Incorrect renderer argument!");
		m_VisionSystemStatus = objectStatus::STANDBY;
		return false;
	}
#else
InnoHighLevelSystem_EXPORT bool InnoVisionSystem::setup()
{
#endif
	WindowSystemSingletonComponent::getInstance().m_windowName = InnoGameSystem::getGameName();

	if (!setupWindow())
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

bool InnoVisionSystem::setupWindow()
{
	if (!WindowSystem::Instance::get().setup())
	{
		return false;
	}
	return true;
}

bool InnoVisionSystem::setupRendering()
{
	if (!RenderingSystem::Instance::get().setup())
	{
		return false;
	}
	RenderingSystemSingletonComponent::getInstance().m_canRender = true;

	return true;
}

bool InnoVisionSystem::setupGui()
{
	if (!GuiSystem::Instance::get().setup())
	{
		return false;
	}
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoVisionSystem::initialize()
{
	WindowSystem::Instance::get().initialize();
	RenderingSystem::Instance::get().initialize();
	GuiSystem::Instance::get().initialize();

	InnoLogSystem::printLog("VisionSystem has been initialized.");
	return true;
}

InnoHighLevelSystem_EXPORT bool InnoVisionSystem::update()
{
	WindowSystem::Instance::get().update();

	if (WindowSystem::Instance::get().getStatus() == objectStatus::ALIVE)
	{
		if (RenderingSystemSingletonComponent::getInstance().m_canRender)
		{
			RenderingSystemSingletonComponent::getInstance().m_canRender = false;
			RenderingSystem::Instance::get().update();
			GuiSystem::Instance::get().update();
			WindowSystem::Instance::get().swapBuffer();
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
	if (WindowSystem::Instance::get().getStatus() == objectStatus::ALIVE)
	{
		GuiSystem::Instance::get().terminate();
		RenderingSystem::Instance::get().terminate();
		WindowSystem::Instance::get().terminate();	
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