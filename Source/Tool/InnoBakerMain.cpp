#include "../../Engine/Engine/Engine.h"
#include "../../Engine/Interface/ILogicClient.h"
#include "InnoBaker.h"

class InnoBakerLogicClient : public ILogicClient
{
	// Inherited via ILogicClient
	bool Setup(ISystemConfig* systemConfig) override
	{
		return true;
	}
	bool Initialize() override
	{
		return true;
	}
	bool Update() override
	{
		return true;
	}
	bool Terminate() override
	{
		return true;
	}
	ObjectStatus GetStatus() override
	{
		return ObjectStatus();
	}
	std::string getApplicationName() override
	{
		return "InnoBaker/";
	}
};

namespace InnoBaker
{
	std::unique_ptr<Engine> m_pModuleManager;
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
	FILE* stream;
	err = freopen_s(&stream, "CONOUT$", "w", stdout);
	SetConsoleTitle("Innocence Engine Debug Console");

	auto appHook = hInstance;
	auto extraHook = nullptr;
#else
int main(int argc, char* argv[])
{
	auto appHook = nullptr;
	auto extraHook = nullptr;
#endif // INNO_PLATFORM_WIN

	m_pModuleManager = std::make_unique<Engine>();
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

	if (!m_pModuleManager.get()->Setup(appHook, extraHook, pScmdline, m_pRenderingClient.get(), m_pLogicClient.get()))
	{
		return false;
	}

	if (!m_pModuleManager->Initialize())
	{
		return false;
	}

	std::string l_windowArguments = pScmdline;

	auto l_bakeStageArgPos = l_windowArguments.find("-bakestage");

	if (l_bakeStageArgPos != std::string::npos)
	{
		auto l_probeArgPos = l_windowArguments.find("probe ");

		if (l_probeArgPos != std::string::npos)
		{
			std::string l_sceneFileName = l_windowArguments.substr(l_probeArgPos + 5);
			l_sceneFileName = l_sceneFileName.substr(1, l_sceneFileName.size() - 1);

			InnoBaker::BakeProbeCache(l_sceneFileName.c_str());
		}

		auto l_brickcacheStageArgPos = l_windowArguments.find("brickcache ");

		if (l_brickcacheStageArgPos != std::string::npos)
		{
			std::string l_surfelCacheFileName = l_windowArguments.substr(l_brickcacheStageArgPos + 10);
			l_surfelCacheFileName = l_surfelCacheFileName.substr(1, l_surfelCacheFileName.size() - 1);

			InnoBaker::BakeBrickCache(l_surfelCacheFileName.c_str());
		}

		auto l_brickStageArgPos = l_windowArguments.find("brick ");

		if (l_brickStageArgPos != std::string::npos)
		{
			std::string l_brickCacheFileName = l_windowArguments.substr(l_brickStageArgPos + 5);
			l_brickCacheFileName = l_brickCacheFileName.substr(1, l_brickCacheFileName.size() - 1);

			InnoBaker::BakeBrick(l_brickCacheFileName.c_str());
		}

		auto l_brickFactorStageArgPos = l_windowArguments.find("brickfactor ");

		if (l_brickFactorStageArgPos != std::string::npos)
		{
			std::string l_brickFileName = l_windowArguments.substr(l_brickFactorStageArgPos + 11);
			l_brickFileName = l_brickFileName.substr(1, l_brickFileName.size() - 1);

			InnoBaker::BakeBrickFactor(l_brickFileName.c_str());
		}
	}

#ifdef TEST
	InnoBaker::BakeProbeCache("..//Res//Scenes//GITest.InnoScene");
	InnoBaker::BakeBrickCache("..//Res//Intermediate//GITest.InnoSurfelCache");
	InnoBaker::BakeBrick("..//Res//Intermediate//GITest.InnoBrickCacheSummary");
	InnoBaker::BakeBrickFactor("..//Res//Scenes//GITest.InnoBrick");
#endif

	m_pModuleManager->Terminate();

	return 0;
}