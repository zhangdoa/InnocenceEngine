#include "InnoApplicationEntry.h"
#include "../../ModuleManager/ModuleManager.h"
#include "../../../Client/ClientMetadata.h"

#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define INNO_RENDERING_CLIENT_HEADER_PATH ../../../Client/RenderingClient/INNO_RENDERING_CLIENT.h
#define INNO_LOGIC_CLIENT_HEADER_PATH ../../../Client/LogicClient/INNO_LOGIC_CLIENT.h

#include STRINGIZE(INNO_RENDERING_CLIENT_HEADER_PATH)
#include STRINGIZE(INNO_LOGIC_CLIENT_HEADER_PATH)

namespace InnoApplicationEntry
{
	std::unique_ptr<InnoModuleManager> m_pModuleManager;
	std::unique_ptr<INNO_RENDERING_CLIENT> m_pRenderingClient;
	std::unique_ptr<INNO_LOGIC_CLIENT> m_pLogicClient;
}

bool InnoApplicationEntry::Setup(void* appHook, void* extraHook, char* pScmdline)
{
	m_pModuleManager = std::make_unique<InnoModuleManager>();
	if (!m_pModuleManager.get())
	{
		return false;
	}

	m_pRenderingClient = std::make_unique<INNO_RENDERING_CLIENT>();
	if (!m_pRenderingClient.get())
	{
		return false;
	}

	m_pLogicClient = std::make_unique<INNO_LOGIC_CLIENT>();
	if (!m_pLogicClient.get())
	{
		return false;
	}

	if (!m_pModuleManager.get()->Setup(appHook, extraHook, pScmdline, m_pRenderingClient.get(), m_pLogicClient.get()))
	{
		return false;
	}

	return true;
}

bool InnoApplicationEntry::Initialize()
{
	return m_pModuleManager->Initialize();
}

bool InnoApplicationEntry::Run()
{
	return m_pModuleManager->Run();
}

bool InnoApplicationEntry::Terminate()
{
	return m_pModuleManager->Terminate();
}