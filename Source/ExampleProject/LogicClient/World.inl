#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Services/CameraService.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/RayTracer/RayTracer.h"
#include "../../Engine/Component/TransformComponent.h"
#include "../../Engine/Component/CameraComponent.h"
#include "../../Engine/Common/IOService.h"

#include "../../Engine/Engine.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

using namespace Inno;

#include "Player.inl"

namespace Inno
{
// ---------------------------------------------------------------------------
// Serialize-determinism test (TASK-111)
// Load a scene, save it in-place, compare saved state against the snapshot
// taken just before the save, restore originals, then exit 0 (pass) or 1
// (diff found).  Only called when -serialize_test <path> is supplied.
// ---------------------------------------------------------------------------
namespace
{
	// Read all regular files under dirPath into a map path→content.
	std::map<std::string, std::string> SnapshotDirectory(const std::string& dirPath)
	{
		std::map<std::string, std::string> result;
		namespace fs = std::filesystem;
		if (!fs::exists(dirPath))
			return result;
		for (auto& entry : fs::directory_iterator(dirPath))
		{
			if (!entry.is_regular_file())
				continue;
			std::string path = entry.path().generic_string();
			std::ifstream f(path, std::ios::binary);
			if (f)
				result[path] = std::string(std::istreambuf_iterator<char>(f), {});
		}
		return result;
	}

	std::string ReadFileContent(const std::string& path)
	{
		std::ifstream f(path, std::ios::binary);
		if (!f)
			return {};
		return std::string(std::istreambuf_iterator<char>(f), {});
	}

	void RestoreFile(const std::string& path, const std::string& content)
	{
		std::ofstream f(path, std::ios::out | std::ios::trunc | std::ios::binary);
		f << content;
	}

	// Returns true if all saved files match the pre-save snapshot; logs
	// every diffing file.  Restores originals from the snapshot on failure.
	bool CompareAndRestore(
		const std::string&                           l_sceneFilePath,
		const std::string&                           l_origScene,
		const std::string&                           l_compDirPath,
		const std::map<std::string, std::string>&    l_origComps)
	{
		size_t l_diffCount = 0;

		// Check scene file
		std::string l_savedScene = ReadFileContent(l_sceneFilePath);
		if (l_savedScene != l_origScene)
		{
			Log(Warning, "[serialize-test] DIFF: ", l_sceneFilePath.c_str());
			++l_diffCount;
		}

		// Check component directory
		auto l_savedComps = SnapshotDirectory(l_compDirPath);
		for (auto& [path, saved] : l_savedComps)
		{
			auto it = l_origComps.find(path);
			if (it == l_origComps.end())
			{
				Log(Warning, "[serialize-test] NEW file created by save: ", path.c_str());
				++l_diffCount;
			}
			else if (it->second != saved)
			{
				Log(Warning, "[serialize-test] DIFF: ", path.c_str());
				++l_diffCount;
			}
		}
		for (auto& [path, _] : l_origComps)
		{
			if (!l_savedComps.count(path))
			{
				Log(Warning, "[serialize-test] DELETED by save: ", path.c_str());
				++l_diffCount;
			}
		}

		// Restore originals so the working tree stays clean. The aggregate
		// Error is emitted after restore — LogService _Exit(1) on Error in
		// test mode would otherwise kill the process before the restore loop
		// runs, leaving Bin/Data drifted across runs.
		if (l_diffCount > 0)
		{
			RestoreFile(l_sceneFilePath, l_origScene);
			for (auto& [path, content] : l_origComps)
				RestoreFile(path, content);
			Log(Error, "[serialize-test] FAILED: ", l_diffCount,
				" file(s) diffed/missing — see Warning lines above. Source files restored.");
		}
		return l_diffCount == 0;
	}

	void RunSerializeTest(const char* l_sceneRelPath)
	{
		auto* io = g_Engine->Get<IOService>();
		std::string l_dataDir    = io->GetDataDirectory();
		std::string l_sceneFile  = l_dataDir + l_sceneRelPath;
		std::string l_compDir    = l_dataDir + io->GetProjectName() + "/Components/";

		// Snapshot before save
		std::string l_origScene = ReadFileContent(l_sceneFile);
		auto        l_origComps = SnapshotDirectory(l_compDir);

		g_Engine->Get<SceneService>()->Save("");

		bool l_passed = CompareAndRestore(l_sceneFile, l_origScene, l_compDir, l_origComps);
		if (l_passed)
		{
			Log(Success, "[serialize-test] PASSED — round-trip is idempotent for: ", l_sceneRelPath);
		}
		// Stash the result in InitConfig; WinMain reads it after Engine::Terminate()
		// returns and uses it as the process exit code. IWindowService::Terminate()
		// ends the main loop cleanly, driving the normal Terminate path which
		// releases D3D12 resources and drains the debug layer.
		g_Engine->setSerializeTestResult(l_passed ? 0 : 1);
		g_Engine->Get<IWindowService>()->Terminate();
	}
} // anonymous namespace

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

		// Scene picker (R/L) and asset import (Y) moved to Editor-Next panels (TASK-62).
		// Engine continues to accept LOAD_SCENE / IMPORT_ASSET via EditorService WebSocket;
		// the editor panes are the user-facing trigger.
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_N, true }, ButtonEvent{ EventLifeTime::OneShot, &f_runRayTracing });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_F, true }, ButtonEvent{ EventLifeTime::OneShot, &f_pauseGame });

		const auto& l_config = g_Engine->getInitConfig();
		if (l_config.serializeTest[0] != '\0')
		{
			// Serialize-determinism mode: run the save+compare test after the
			// scene has finished loading, then exit.  No player, no game loop.
			const char* l_testScene = l_config.serializeTest;
			f_sceneLoadingFinishedCallback = [l_testScene]() {
				RunSerializeTest(l_testScene);
			};
		}
		else
		{
			f_sceneLoadingFinishedCallback = [&]() {
				if (!m_player)
					m_player = new Player();
				m_player->Setup();
				m_ObjectStatus = ObjectStatus::Activated;
			};
		}

		g_Engine->Get<SceneService>()->AddSceneLoadedCallback(&f_sceneLoadingFinishedCallback);

		return true;
	}

	bool WorldSystem::Initialize()
	{
		bool l_result = true;

		// Initial-scene selection priority:
		//   1. -serialize_test target  (test runs save+compare on this scene then exits)
		//   2. -scene override         (three-scene capture harness picks the scene)
		//   3. UnitTest default        (normal engine operation)
		// When (2) is set, Update() also suppresses the default frame-5
		// auto-switch to GISponza so the chosen scene renders end-to-end.
		const auto& l_config = g_Engine->getInitConfig();
		const char* l_initialScene = "ExampleProject/Scenes/UnitTest.InnoScene";
		if (l_config.serializeTest[0] != '\0')
			l_initialScene = l_config.serializeTest;
		else if (l_config.initialScene[0] != '\0')
			l_initialScene = l_config.initialScene;
		g_Engine->Get<SceneService>()->Load(l_initialScene, true);

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

			// Default auto-test transition: render the UnitTest startup
			// scene for a few frames, then swap to GISponza for the rest of
			// the run. Suppressed when -scene <path> picks a target scene
			// explicitly — the three-scene capture harness drives each
			// scene end-to-end via that override.
			const bool l_sceneOverridden = g_Engine->getInitConfig().initialScene[0] != '\0';
			if (!l_sceneOverridden && !m_AutoGISceneTriggered && m_AutoFrameCount >= 5)
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

			// Camera orbit override — TASK-124 [B], TASK-213 CL C.
			// Each frame in [0, cameraOrbitDuration] stomps the Main Camera
			// transform with a yaw that sweeps 0→360° over the duration, at
			// the configured pitch and radius around the world origin. Runs
			// after the scene-load trigger so the Main Camera is guaranteed
			// to exist by the time we try to find it. Player's Update()
			// further below will stomp this on windowed runs where the
			// player is driving — accepted for v1 (orbit is intended for
			// -offscreen capture sessions).
			//
			// TASK-213 CL C: yaw is now driven by FMS::GetSteadyStateRelative
			// FrameCount(), not m_AutoFrameCount. Reason — m_AutoFrameCount
			// advances unconditionally post-Activated, so its value at any
			// given dump frame depends on the deferred-init drain timing
			// (RC-6 in the task design pass). The steady-state-relative
			// counter is gated on the same latch as m_autoCaptureFrameCount,
			// so yaw at dump frame N is independent of the variable load-
			// frame count → identical viewpoint at the same dump frame
			// across binaries.
			const auto& l_initCfg = g_Engine->getInitConfig();
			const uint32_t l_orbitFrame =
				g_Engine->Get<FrameManagementService>()->GetSteadyStateRelativeFrameCount();
			if (l_initCfg.cameraOrbitActive
				&& l_orbitFrame <= static_cast<uint32_t>(l_initCfg.cameraOrbitDuration))
			{
				auto l_Registry = g_Engine->Get<EntityRegistry>();
				auto l_CameraEntity = l_Registry->FindByName("Main Camera");
				if (l_CameraEntity != INVALID_ENTITY)
				{
					auto* l_CameraTransform = l_Registry->Get<TransformComponent>(l_CameraEntity);
					if (l_CameraTransform)
					{
						const float l_yawDeg = 360.0f
							* static_cast<float>(l_orbitFrame)
							/ static_cast<float>(l_initCfg.cameraOrbitDuration);
						const float l_pitchRad = l_initCfg.cameraOrbitPitchDeg * PI<float> / 180.0f;
						const float l_yawRad   = l_yawDeg                       * PI<float> / 180.0f;
						const float l_r        = l_initCfg.cameraOrbitRadius;
						const float l_cosP     = std::cos(l_pitchRad);

						l_CameraTransform->m_LocalPos = Vec3(
							l_r * l_cosP * std::sin(l_yawRad),
							l_r * std::sin(l_pitchRad),
							l_r * l_cosP * std::cos(l_yawRad));

						// Default camera forward is -Z, up is +Y. Yaw around
						// world Y brings -Z to face origin; pitch around local
						// X tilts the camera to look at origin when elevated.
						Vec4 l_yawQuat   = Math::getQuatRotator(Vec4(0.0f, 1.0f, 0.0f, 0.0f),  l_yawDeg);
						Vec4 l_pitchQuat = Math::getQuatRotator(Vec4(1.0f, 0.0f, 0.0f, 0.0f), -l_initCfg.cameraOrbitPitchDeg);
						l_CameraTransform->m_LocalRot = l_yawQuat.quatMul(l_pitchQuat);
					}
				}
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
