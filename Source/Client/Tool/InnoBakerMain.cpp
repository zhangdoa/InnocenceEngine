#include "../../Engine/ModuleManager/ModuleManager.h"
#include "../../Engine/Core/ILogicClient.h"
#include "InnoBaker.h"

class InnoBakerLogicClient : public ILogicClient
{
	// Inherited via ILogicClient
	virtual bool setup() override
	{
		return true;
	}
	virtual bool initialize() override
	{
		return true;
	}
	virtual bool update() override
	{
		return true;
	}
	virtual bool terminate() override
	{
		return true;
	}
	virtual ObjectStatus getStatus() override
	{
		return ObjectStatus();
	}
	virtual std::string getApplicationName() override
	{
		return "InnoBaker";
	}
};

namespace InnoBaker
{
	std::unique_ptr<InnoModuleManager> m_pModuleManager;
	std::unique_ptr<InnoBakerRenderingClient> m_pRenderingClient;
	std::unique_ptr<InnoBakerLogicClient> m_pLogicClient;
}

using namespace InnoBaker;

#ifdef INNO_PLATFORM_WIN
#include <windows.h>
#include <windowsx.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int nCmdshow)
{
	AllocConsole();

	errno_t err;
	FILE *stream;
	err = freopen_s(&stream, "CONOUT$", "w", stdout);
	SetConsoleTitle("Innocence Engine Debug Console");

	auto appHook = hInstance;
	auto extraHook = nullptr;
#else
int main(int argc, char *argv[])
{
	auto appHook = nullptr;
	auto extraHook = nullptr;
#endif // INNO_PLATFORM_WIN

	m_pModuleManager = std::make_unique<InnoModuleManager>();
	if (!m_pModuleManager.get())
	{
		return false;
	}

	m_pRenderingClient = std::make_unique<InnoBakerRenderingClient>();
	if (!m_pRenderingClient.get())
	{
		return false;
	}

	m_pLogicClient = std::make_unique<InnoBakerLogicClient>();
	if (!m_pLogicClient.get())
	{
		return false;
	}

	if (!m_pModuleManager.get()->setup(appHook, extraHook, pScmdline, m_pRenderingClient.get(), m_pLogicClient.get()))
	{
		return false;
	}

	if (!m_pModuleManager->initialize())
	{
		return false;
	}

	m_pModuleManager->run();
	m_pModuleManager->terminate();

	return 0;
}