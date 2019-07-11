#include "InnoApplication.h"
#include "../ModuleManager/ModuleManager.h"
#include "../../Game/GameInstance.h"

namespace InnoApplication
{
	std::unique_ptr<InnoModuleManager> m_pModuleManager;
	std::unique_ptr<GameInstance> m_pGameInstance;
}

bool InnoApplication::Setup(void* appHook, void* extraHook, char* pScmdline)
{
	m_pModuleManager = std::make_unique<InnoModuleManager>();
	if (!m_pModuleManager.get())
	{
		return false;
	}

	m_pGameInstance = std::make_unique<GameInstance>();
	if (!m_pGameInstance.get())
	{
		return false;
	}

	if (!m_pModuleManager.get()->setup(appHook, extraHook, pScmdline, m_pGameInstance.get()))
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