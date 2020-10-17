#include "ApplicationEntry.h"
#include "../../Engine/Engine.h"
#include "../../../Client/ClientMetadata.h"

#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define INNO_RENDERING_CLIENT_HEADER_PATH ../../../Client/RenderingClient/INNO_RENDERING_CLIENT.h
#define INNO_LOGIC_CLIENT_HEADER_PATH ../../../Client/LogicClient/INNO_LOGIC_CLIENT.h

#include STRINGIZE(INNO_RENDERING_CLIENT_HEADER_PATH)
#include STRINGIZE(INNO_LOGIC_CLIENT_HEADER_PATH)

using namespace Inno;
namespace Inno
{
	namespace ApplicationEntry
	{
		std::unique_ptr<Engine> m_pEngine;
		std::unique_ptr<INNO_RENDERING_CLIENT> m_pRenderingClient;
		std::unique_ptr<INNO_LOGIC_CLIENT> m_pLogicClient;
	}
}

bool ApplicationEntry::Setup(void* appHook, void* extraHook, char* pScmdline)
{
	m_pEngine = std::make_unique<Engine>();
	if (!m_pEngine.get())
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

	if (!m_pEngine->Setup(appHook, extraHook, pScmdline, m_pRenderingClient.get(), m_pLogicClient.get()))
	{
		return false;
	}

	return true;
}

bool ApplicationEntry::Initialize()
{
	return m_pEngine->Initialize();
}

bool ApplicationEntry::Run()
{
	return m_pEngine->Run();
}

bool ApplicationEntry::Terminate()
{
	return m_pEngine->Terminate();
}