#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Services/CameraService.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/AssetService.h"
#include "../../Engine/RayTracer/RayTracer.h"
#include "../../Engine/Component/TransformComponent.h"
#include "../../Engine/Component/CameraComponent.h"

#include "../../Engine/Engine.h"

using namespace Inno;

#include "Player.inl"

namespace Inno
{
	class WorldSystem : public IService
	{
	public:
		INNO_CLASS_CONCRETE_DEFAULT(WorldSystem);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update();
		bool Terminate() override;
		ObjectStatus GetStatus() { return m_ObjectStatus; }

	private:
		void runTest(uint32_t testTime, std::function<bool()> testCase);

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		Player* m_player = nullptr;

		std::function<void()> f_sceneLoadingFinishedCallback;
		std::function<void()> f_loadTestScene;
		std::function<void()> f_loadGISponza;
		std::function<void()> f_loadGITestBox;
		std::function<void()> f_convertModel;

		std::function<void()> f_runRayTracing;
		std::function<void()> f_pauseGame;

		bool allowUpdate = true;

		uint32_t m_AutoFrameCount = 0;
		bool m_AutoGISceneTriggered = false;
		bool m_AutoReloadTriggered = false;
		bool m_AutoTerminateCalled = false;
	};

	bool WorldSystem::Setup(IServiceConfig* systemConfig)
	{
		std::default_random_engine l_generator;
		auto l_testQuatToMat = [&]() -> bool {
			std::uniform_real_distribution<float> randomAxis(0.0f, 1.0f);
			auto axisSample = Vec4(randomAxis(l_generator) * 2.0f - 1.0f, randomAxis(l_generator) * 2.0f - 1.0f, randomAxis(l_generator) * 2.0f - 1.0f, 0.0f);
			axisSample = axisSample.normalize();

			std::uniform_real_distribution<float> randomAngle(0.0f, 360.0f);
			auto angleSample = randomAngle(l_generator);

			Vec4 originalRot = Math::getQuatRotator(axisSample, angleSample);
			Mat4 rotMat = Math::toRotationMatrix(originalRot);
			auto resultRot = Math::toQuatRotator(rotMat);

			auto testResult = true;
			testResult &= (std::abs(std::abs(originalRot.w) - std::abs(resultRot.w)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.x) - std::abs(resultRot.x)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.y) - std::abs(resultRot.y)) < epsilon<float, 4>);
			testResult &= (std::abs(std::abs(originalRot.z) - std::abs(resultRot.z)) < epsilon<float, 4>);

			return testResult;
			};

		runTest(512, l_testQuatToMat);

		f_runRayTracing = [&]() { g_Engine->Get<RayTracer>()->Execute(); };
		f_pauseGame = [&]() { allowUpdate = !allowUpdate; };

		f_loadTestScene = []() {
			g_Engine->Get<SceneService>()->Load("ExampleProject/Scenes/UnitTest.InnoScene", true);
			};

		f_loadGISponza = []() {
			g_Engine->Get<SceneService>()->Load("ExampleProject/Scenes/GISponza.InnoScene", true);
			};

		f_loadGITestBox = []() {
			g_Engine->Get<SceneService>()->Load("ExampleProject/Scenes/GITestBox.InnoScene", true);
			};

		f_convertModel = []() {
			g_Engine->Get<AssetService>()->Import("../OriginalAssets/Models/Sponza_PBR/main1_sponza/NewSponza_Main_glTF_003.gltf");
			g_Engine->Get<AssetService>()->Import("../OriginalAssets/Models/Sponza_Curtains/pkg_a_curtains/NewSponza_Curtains_glTF.gltf");
			g_Engine->Get<AssetService>()->Import("../OriginalAssets/Models/orb/ShaderBall.fbx");
			g_Engine->Get<AssetService>()->Import("../OriginalAssets/Models/bunny/bunny.ply");
			g_Engine->Get<AssetService>()->Import("../OriginalAssets/Models/dragon/dragon.ply");
			};

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_N, true }, ButtonEvent{ EventLifeTime::OneShot, &f_runRayTracing });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_F, true }, ButtonEvent{ EventLifeTime::OneShot, &f_pauseGame });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_R, true }, ButtonEvent{ EventLifeTime::OneShot, &f_loadTestScene });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_L, true }, ButtonEvent{ EventLifeTime::OneShot, &f_loadGISponza });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_B, true }, ButtonEvent{ EventLifeTime::OneShot, &f_loadGITestBox });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_Y, true }, ButtonEvent{ EventLifeTime::OneShot, &f_convertModel });

		f_sceneLoadingFinishedCallback = [&]() {
			if (!m_player)
				m_player = new Player();
			m_player->Setup();
			m_ObjectStatus = ObjectStatus::Activated;
			};

		g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadingFinishedCallback);

		return true;
	}

	bool WorldSystem::Initialize()
	{
		bool l_result = true;

		g_Engine->Get<SceneService>()->Load("ExampleProject/Scenes/UnitTest.InnoScene");

		  RayTracerConfig l_cfg;
		l_cfg.downsampleDenominator = 2;
		auto* l_rayTracer = g_Engine->Get<RayTracer>();
		l_rayTracer->Setup(&l_cfg);
		l_rayTracer->Initialize();

		return true;
	}

	bool WorldSystem::Update()
	{
		if (m_ObjectStatus != ObjectStatus::Activated)
			return false;

		auto l_totalFrames = g_Engine->getInitConfig().totalFrames;
		if (l_totalFrames > 0)
		{
			m_AutoFrameCount++;

			if (!m_AutoGISceneTriggered && m_AutoFrameCount >= 5)
			{
				m_AutoGISceneTriggered = true;
				g_Engine->Get<SceneService>()->Load("ExampleProject/Scenes/GISponza.InnoScene", true);
				Log(Success, "Auto-test: loaded GISponza scene at frame ", m_AutoFrameCount, ".");
			}

			auto l_reloadAtFrame = g_Engine->getInitConfig().reloadAtFrame;
			if (l_reloadAtFrame > 0 && !m_AutoReloadTriggered && m_AutoFrameCount >= static_cast<uint32_t>(l_reloadAtFrame))
			{
				m_AutoReloadTriggered = true;
				g_Engine->Get<SceneService>()->Load("ExampleProject/Scenes/UnitTest.InnoScene", true);
				Log(Success, "Auto-test: reload triggered at frame ", m_AutoFrameCount, ", switching back to UnitTest scene.");
			}

			if (!m_AutoTerminateCalled && m_AutoFrameCount >= static_cast<uint32_t>(l_totalFrames))
			{
				m_AutoTerminateCalled = true;
				Log(Success, "Auto-test: ", l_totalFrames, " frames rendered, terminating.");
				g_Engine->Get<IWindowService>()->Terminate();
			}
		}

		if (!allowUpdate)
			return false;

		if (m_player)
			m_player->Update(0.0f);

		return true;
	}

	bool WorldSystem::Terminate()
	{
		if (m_player)
		{
			m_player->Terminate();
			delete m_player;
		}

		if (g_Engine->getInitConfig().totalFrames > 0)
		{
			Log(Verbose, "Auto-test: running CPU path tracer reference render...");
			g_Engine->Get<RayTracer>()->Execute();
		}

		g_Engine->Get<RayTracer>()->Terminate();

		m_ObjectStatus = ObjectStatus::Terminated;
		return true;
	}

	void WorldSystem::runTest(uint32_t testTime, std::function<bool()> testCase)
	{
		Log(Verbose, "Start test...");
		for (uint32_t i = 0; i < testTime; i++)
		{
			auto l_result = testCase();
			if (!l_result)
			{
				Log(Warning, "Test failure.");
			}
		}

		Log(Verbose, "Finished test for ", testTime, " times.");
	}

}
