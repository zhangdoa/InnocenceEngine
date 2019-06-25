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
	if (!m_pModuleManager->initialize())
	{
		return false;
	}

	return true;
}

bool InnoApplication::Run()
{
	if (!m_pModuleManager->run())
	{
		return false;
	}
	return true;
}

bool InnoApplication::Terminate()
{
	if (!m_pModuleManager->terminate())
	{
		return false;
	}
	return true;
}