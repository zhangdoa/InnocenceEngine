#include "Baker.h"
#include "../../Client/DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Common/MathHelper.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

#include "../../Engine/Core/IOService.h"

#include "Serializer.h"
#include "ProbeGenerator.h"
#include "SurfelGenerator.h"
#include "BrickGenerator.h"

std::string Baker::Config::parseFileName(const char* fileName)
{
	auto l_fileName = std::string(fileName);
	auto l_startOffset = l_fileName.find_last_of("/");
	auto l_endOffset = l_fileName.find_last_of(".");
	auto l_result = l_fileName.substr(l_startOffset + 1, l_endOffset - l_startOffset - 1);

	return l_result;
}

void Baker::Setup()
{
	ProbeGenerator::Get().setup();
	SurfelGenerator::Get().setup();
	BrickGenerator::Get().setup();
}

void Baker::BakeProbeCache(const char* sceneName)
{
	g_Engine->getSceneSystem()->loadScene(sceneName, false);
	auto l_playerCameraEntity = g_Engine->getEntityManager()->Find("playerCharacterCamera");
	if (l_playerCameraEntity.has_value())
	{
		auto l_playerCameraComponent = g_Engine->getComponentManager()->Find<CameraComponent>(l_playerCameraEntity.value());
        static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->SetMainCamera(l_playerCameraComponent);
        static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->SetActiveCamera(l_playerCameraComponent);
	}

	g_Engine->Update();
	Config::Get().m_exportFileName = g_Engine->getSceneSystem()->getCurrentSceneName();

	std::vector<Probe> l_probes;

	auto l_BakerProbeCacheTask = g_Engine->getTaskSystem()->Submit("BakerProbeCacheTask", 2, nullptr,
		[&]() {
			ProbeGenerator::Get().gatherStaticMeshData();
			ProbeGenerator::Get().generateProbeCaches(l_probes);
			SurfelGenerator::Get().captureSurfels(l_probes);
		});

	l_BakerProbeCacheTask.m_Future->Get();
}

void Baker::BakeBrickCache(const char* surfelCacheFileName)
{
	auto l_filePath = g_Engine->getFileSystem()->getWorkingDirectory();
	Config::Get().m_exportFileName = Config::Get().parseFileName(surfelCacheFileName);

	std::ifstream l_surfelCacheFile;

	l_surfelCacheFile.open(l_filePath + surfelCacheFileName, std::ios::binary);

	std::vector<Surfel> l_surfelCaches;

	if (l_surfelCacheFile.is_open())
	{
		IOService::deserializeVector(l_surfelCacheFile, l_surfelCaches);
		BrickGenerator::Get().generateBrickCaches(l_surfelCaches);
	}
	else
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "Baker: Surfel cache file not exists!");
	}
}

void Baker::BakeBrick(const char* brickCacheFileName)
{
	auto l_filePath = g_Engine->getFileSystem()->getWorkingDirectory();
	Config::Get().m_exportFileName = Config::Get().parseFileName(brickCacheFileName);

	std::ifstream l_brickCacheSummaryFile;

	l_brickCacheSummaryFile.open(l_filePath + brickCacheFileName, std::ios::binary);

	if (l_brickCacheSummaryFile.is_open())
	{
		std::vector<BrickCacheSummary> l_brickCacheSummaries;
		std::vector<BrickCache> l_brickCaches;

		IOService::deserializeVector(l_brickCacheSummaryFile, l_brickCacheSummaries);

		deserializeBrickCaches(l_brickCacheSummaries, l_brickCaches);
		BrickGenerator::Get().generateBricks(l_brickCaches);
	}
	else
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "Baker: Brick cache file not exists!");
	}
}

void Baker::BakeBrickFactor(const char* brickFileName)
{
	auto l_filePath = g_Engine->getFileSystem()->getWorkingDirectory();
	Config::Get().m_exportFileName = Config::Get().parseFileName(brickFileName);

	std::ifstream l_brickFile;

	l_brickFile.open(l_filePath + brickFileName, std::ios::binary);

	if (l_brickFile.is_open())
	{
		std::vector<Brick> l_bricks;

		IOService::deserializeVector(l_brickFile, l_bricks);

		l_brickFile.close();

		std::ifstream l_probeFile;

		l_probeFile.open(l_filePath + "..//Res//Scenes//" + Config::Get().m_exportFileName + ".Probe", std::ios::binary);

		if (l_probeFile.is_open())
		{
			std::vector<Probe> l_probes;

			IOService::deserializeVector(l_probeFile, l_probes);

			l_probeFile.close();

			auto l_BakerBrickFactorTask = g_Engine->getTaskSystem()->Submit("BakerBrickFactorTask", 2, nullptr,
				[&]() {
					BrickGenerator::Get().assignBrickFactorToProbesByGPU(l_bricks, l_probes);
				});

			l_BakerBrickFactorTask.m_Future->Get();
		}
		else
		{
			g_Engine->getLogSystem()->Log(LogLevel::Error, "Baker: Probe cache file not exists!");
		}
	}
	else
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "Baker: Brick file not exists!");
	}
}

bool BakerRenderingClient::Setup(ISystemConfig* systemConfig)
{
	auto l_BakerRenderingClientSetupTask = g_Engine->getTaskSystem()->Submit("BakerRenderingClientSetupTask", 2, nullptr,
		[]() {
			DefaultGPUBuffers::Setup();
			Baker::Setup();
		});
	l_BakerRenderingClientSetupTask.m_Future->Get();

	return true;
}

bool BakerRenderingClient::Initialize()
{
	auto l_BakerRenderingClientInitializeTask = g_Engine->getTaskSystem()->Submit("BakerRenderingClientInitializeTask", 2, nullptr,
		[]() {
			DefaultGPUBuffers::Initialize();
		});
	l_BakerRenderingClientInitializeTask.m_Future->Get();

	return true;
}

bool BakerRenderingClient::Render(IRenderingConfig* renderingConfig)
{
	return true;
}

bool BakerRenderingClient::Terminate()
{
	DefaultGPUBuffers::Terminate();

	return true;
}

ObjectStatus BakerRenderingClient::GetStatus()
{
	return ObjectStatus();
}