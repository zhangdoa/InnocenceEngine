#include "InnoApplication.h"
#include "../ModuleManager/ModuleManager.h"
#include "../../Client/RenderingClient/DefaultRenderingClient.h"
#include "../../Client/LogicClient/DefaultGameClient.h"

namespace InnoApplication
{
	std::unique_ptr<InnoModuleManager> m_pModuleManager;
	std::unique_ptr<DefaultRenderingClient> m_pRenderingClient;
	std::unique_ptr<DefaultGameClient> m_pLogicClient;
}

bool InnoApplication::Setup(void* appHook, void* extraHook, char* pScmdline)
{
	m_pModuleManager = std::make_unique<InnoModuleManager>();
	if (!m_pModuleManager.get())
	{
		return false;
	}

	m_pRenderingClient = std::make_unique<DefaultRenderingClient>();
	if (!m_pRenderingClient.get())
	{
		return false;
	}

	m_pLogicClient = std::make_unique<DefaultGameClient>();
	if (!m_pLogicClient.get())
	{
		return false;
	}

	if (!m_pModuleManager.get()->setup(appHook, extraHook, pScmdline, m_pRenderingClient.get(), m_pLogicClient.get()))
	{
		return false;
	}

	return true;
}

bool InnoApplication::Initialize()
{
	return m_pModuleManager->initialize();
}

bool InnoApplication::Run()
{
	return m_pModuleManager->run();
}

bool InnoApplication::Terminate()
{
	return m_pModuleManager->terminate();
}