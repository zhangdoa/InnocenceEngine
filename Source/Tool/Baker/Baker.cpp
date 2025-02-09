#include "Baker.h"

#include "../../Engine/Common/MathHelper.h"
#include "../../Engine/Common/IOService.h"
#include "../../Engine/Common/TaskScheduler.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/EntityManager.h"
#include "../../Engine/Services/ComponentManager.h"

#include "../../Engine/Engine.h"

using namespace Inno;


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
	g_Engine->Get<SceneService>()->Load(sceneName, false);
	auto l_playerCameraEntity = g_Engine->Get<EntityManager>()->Find("playerCharacterCamera");
	if (l_playerCameraEntity.has_value())
	{
		auto l_playerCameraComponent = g_Engine->Get<ComponentManager>()->Find<CameraComponent>(l_playerCameraEntity.value());
        static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->SetMainCamera(l_playerCameraComponent);
        static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->SetActiveCamera(l_playerCameraComponent);
	}

	g_Engine->Update();
	Config::Get().m_exportFileName = g_Engine->Get<SceneService>()->GetCurrentSceneName();

	std::vector<Probe> l_probes;

	auto l_BakerProbeCacheTask = g_Engine->Get<TaskScheduler>()->Submit("BakerProbeCacheTask", 2,
		[&]() {
			ProbeGenerator::Get().gatherStaticMeshData();
			ProbeGenerator::Get().generateProbeCaches(l_probes);
			SurfelGenerator::Get().captureSurfels(l_probes);
		});

	l_BakerProbeCacheTask->Wait();
}

void Baker::BakeBrickCache(const char* surfelCacheFileName)
{
	auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();
	Config::Get().m_exportFileName = Config::Get().parseFileName(surfelCacheFileName);

	std::ifstream l_surfelCacheFile;

	l_surfelCacheFile.open(l_filePath + surfelCacheFileName, std::ios::binary);

	std::vector<Surfel> l_surfelCaches;

	if (l_surfelCacheFile.is_open())
	{
		g_Engine->Get<IOService>()->deserializeVector(l_surfelCacheFile, l_surfelCaches);
		BrickGenerator::Get().generateBrickCaches(l_surfelCaches);
	}
	else
	{
		Log(Error, "Surfel cache file not exists!");
	}
}

void Baker::BakeBrick(const char* brickCacheFileName)
{
	auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();
	Config::Get().m_exportFileName = Config::Get().parseFileName(brickCacheFileName);

	std::ifstream l_brickCacheSummaryFile;

	l_brickCacheSummaryFile.open(l_filePath + brickCacheFileName, std::ios::binary);

	if (l_brickCacheSummaryFile.is_open())
	{
		std::vector<BrickCacheSummary> l_brickCacheSummaries;
		std::vector<BrickCache> l_brickCaches;

		g_Engine->Get<IOService>()->deserializeVector(l_brickCacheSummaryFile, l_brickCacheSummaries);

		deserializeBrickCaches(l_brickCacheSummaries, l_brickCaches);
		BrickGenerator::Get().generateBricks(l_brickCaches);
	}
	else
	{
		Log(Error, "Brick cache file not exists!");
	}
}

void Baker::BakeBrickFactor(const char* brickFileName)
{
	auto l_filePath = g_Engine->Get<IOService>()->getWorkingDirectory();
	Config::Get().m_exportFileName = Config::Get().parseFileName(brickFileName);

	std::ifstream l_brickFile;

	l_brickFile.open(l_filePath + brickFileName, std::ios::binary);

	if (l_brickFile.is_open())
	{
		std::vector<Brick> l_bricks;

		g_Engine->Get<IOService>()->deserializeVector(l_brickFile, l_bricks);

		l_brickFile.close();

		std::ifstream l_probeFile;

		l_probeFile.open(l_filePath + "..//Res//Scenes//" + Config::Get().m_exportFileName + ".Probe", std::ios::binary);

		if (l_probeFile.is_open())
		{
			std::vector<Probe> l_probes;

			g_Engine->Get<IOService>()->deserializeVector(l_probeFile, l_probes);

			l_probeFile.close();

			auto l_BakerBrickFactorTask = g_Engine->Get<TaskScheduler>()->Submit("BakerBrickFactorTask", 2,
				[&]() {
					BrickGenerator::Get().assignBrickFactorToProbesByGPU(l_bricks, l_probes);
				});

			l_BakerBrickFactorTask->Wait();
		}
		else
		{
			Log(Error, "Probe cache file not exists!");
		}
	}
	else
	{
		Log(Error, "Brick file not exists!");
	}
}

bool BakerRenderingClient::Setup(ISystemConfig* systemConfig)
{
	auto l_BakerRenderingClientSetupTask = g_Engine->Get<TaskScheduler>()->Submit("BakerRenderingClientSetupTask", 2,
		[]() {
			Baker::Setup();
		});
	l_BakerRenderingClientSetupTask->Wait();

	return true;
}

bool BakerRenderingClient::Initialize()
{
	return true;
}

bool BakerRenderingClient::Render(IRenderingConfig* renderingConfig)
{
	return true;
}

bool BakerRenderingClient::Terminate()
{
	return true;
}

ObjectStatus BakerRenderingClient::GetStatus()
{
	return ObjectStatus();
}