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
		return "InnoBaker/";
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

	std::string l_windowArguments = pScmdline;

	auto l_bakeStageArgPos = l_windowArguments.find("-bakestage");

	if (l_bakeStageArgPos != std::string::npos)
	{
		auto l_probeArgPos = l_windowArguments.find("probe");

		if (l_probeArgPos != std::string::npos)
		{
			std::string l_sceneFileName = l_windowArguments.substr(l_probeArgPos + 5);
			l_sceneFileName = l_sceneFileName.substr(1, l_sceneFileName.size() - 1);

			InnoBaker::BakeProbeCache(l_sceneFileName);
		}

		auto l_brickcacheStageArgPos = l_windowArguments.find("brickcache");

		if (l_brickcacheStageArgPos != std::string::npos)
		{
			std::string l_surfelCacheFileName = l_windowArguments.substr(l_brickcacheStageArgPos + 10);
			l_surfelCacheFileName = l_surfelCacheFileName.substr(1, l_surfelCacheFileName.size() - 1);

			InnoBaker::BakeBrickCache(l_surfelCacheFileName);
		}

		auto l_brickStageArgPos = l_windowArguments.find("brick");

		if (l_brickStageArgPos != std::string::npos)
		{
			std::string l_brickCacheFileName = l_windowArguments.substr(l_brickStageArgPos + 5);
			l_brickCacheFileName = l_brickCacheFileName.substr(1, l_brickCacheFileName.size() - 1);

			InnoBaker::BakeBrick(l_brickCacheFileName);
		}

		auto l_brickfactorStageArgPos = l_windowArguments.find("brickfactor");

		if (l_brickfactorStageArgPos != std::string::npos)
		{
			std::string l_brickFileName = l_windowArguments.substr(l_brickfactorStageArgPos + 11);
			l_brickFileName = l_brickFileName.substr(1, l_brickFileName.size() - 1);

			auto l_probeCacheFileNameArgPos = l_windowArguments.find("probecache");

			if (l_probeCacheFileNameArgPos != std::string::npos)
			{
				std::string l_probeCacheFileName = l_windowArguments.substr(l_probeCacheFileNameArgPos + 10);
				l_probeCacheFileName = l_probeCacheFileName.substr(1, l_probeCacheFileName.size() - 1);

				InnoBaker::BakeBrickFactor(l_brickFileName, l_probeCacheFileName);
			}
			else
			{
				m_pModuleManager.get()->getLogSystem()->Log(LogLevel::Error, "No probe caches file specified!");
				return -1;
			}
		}
	}
	m_pModuleManager->run();
	m_pModuleManager->terminate();

	return 0;
}