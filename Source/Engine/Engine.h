#pragma once
#include "Common/ClassTemplate.h"
#include "Common/LogService.h"
#include "Interface/IService.h"
#include "Interface/IRenderingClient.h"
#include "Interface/ILogicClient.h"
#include <type_traits>
#include <shared_mutex>

namespace Inno
{
	enum EngineMode { Host, Slave, Sidecar };

	enum GraphicsService { DX12, VK, MT };

	struct InitConfig
	{
		EngineMode engineMode = EngineMode::Host;
		GraphicsService graphicsService = GraphicsService::DX12;
		LogLevel logLevel = LogLevel::Success;
		bool isHeadless = false;
		bool isOffscreen = false;
		bool isAudit = false;
		char testCase[64] = {};
		int maxFrames = 0;  // >0: auto-terminate after this many frames post-GI-scene-load
		uint32_t parentPID = 0; // if >0, we duplicate handles to this process
		int totalFrames = 0;  // >0: auto-terminate after this many frames
		int reloadAtFrame = 0;  // >0: trigger scene reload at this frame
		int captureFrame = -1;  // >=0: trigger RenderDoc/PIX capture at this frame
		// Frame-sequence dump for temporal / cross-frame visual validation.
		// `-dump_frames START-END` writes `gpu_output_NNNN.png` for every
		// frame N in the inclusive range. Lets a reviewer scrub / diff
		// consecutive frames to catch flickering, probe-spawn oscillation,
		// denoiser stability issues — things a single-frame capture misses.
		int dumpFramesStart = -1;
		int dumpFramesEnd = -1;
		// Camera-orbit override for cross-angle visual validation.
		// `-camera_orbit PITCH,RADIUS,DURATION` animates the Main Camera in
		// a horizontal orbit around the world origin: yaw sweeps 0→2π over
		// DURATION frames, pitch is elevation in degrees (0 = horizon,
		// +up / -down), RADIUS is orbit distance from origin in world
		// units, DURATION is the number of frames the orbit runs.
		// DURATION <= 0 disables the override. Pairs with -dump_frames
		// to produce a multi-angle × multi-frame evidence matrix.
		bool cameraOrbitActive = false;
		float cameraOrbitPitchDeg = 0.0f;
		float cameraOrbitRadius = 0.0f;
		int   cameraOrbitDuration = 0;
		bool enableGPUValidation = false;  // enable D3D12 GPU-based validation + sync queue validation
		bool enableGpuTimerLog = false;    // -gpu_timer_log: opt-in per-pass GPU timer Verbose dump (silent by default)
		// Bake mode: run a one-shot asset-import-then-exit pass with no rendering
		// services or window. `-bake "path1;path2;..."` sets isBakeMode=true,
		// copies the `;`-separated list into bakeInputs, and implies isHeadless.
		bool isBakeMode = false;
		char bakeInputs[1024] = {};
		// Serialize-determinism test (TASK-111). When non-empty, Main.exe
		// loads the scene, issues SceneService::Save, and exits. Callers
		// can `git diff Data/` afterwards to see whether the save round-
		// trip preserved the on-disk state; a clean diff means the
		// serializer is idempotent for the tested scene.
		char serializeTest[512] = {};
		// Serialize-test exit code, populated by RunSerializeTest at the
		// end of the save-compare round-trip (0 = pass, 1 = diff detected).
		// Read from WinMain after Engine::Terminate() returns.
		int serializeTestResult = 0;
	};

	class IWindowService;

	class EngineImpl;
	class Engine
	{
	public:
		Engine();
		~Engine();

		bool Setup(
			void* appHook,
			void* extraHook,
			char* pScmdline,
			std::unique_ptr<IRenderingClient> renderingClient,
			std::unique_ptr<ILogicClient> logicClient);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool Run();

		ObjectStatus GetStatus();

		InitConfig getInitConfig();
		void setSerializeTestResult(int result);
		const FixedSizeString<128>& GetApplicationName();
		IWindowService* getWindowService();
		float getTickTime();

		template <typename T>
		T* Get()
		{
			auto type = std::type_index(typeid(T));
			{
				// Fast path: shared read lock for the common hit-in-map case.
				std::shared_lock<std::shared_mutex> lock(singletons_mutex_);
				auto it = singletons_.find(type);
				if (it != singletons_.end())
					return static_cast<T*>(it->second);
			}

			// Miss. TASK-39 fix — previously the bare std::unordered_map was
			// concurrently readable+writable without a mutex, so a worker thread
			// calling Get<LogService>() during startup could race the main thread's
			// populating Emplace and fault at 0x10 inside std::_Find_last.
			//
			// Constructors of many services call Log() which re-enters Get<>, and
			// std::shared_mutex on MSVC (SRW-backed) forbids a thread holding the
			// unique lock from also taking the shared lock. So new T() must run
			// OUTSIDE the unique lock, with a losing-race insert handled by delete.
			if constexpr (std::is_base_of_v<IService, T>) {
				return GetSystemWithDependencies<T>();
			}
			else {
				T* instance = new T();

				std::unique_lock<std::shared_mutex> lock(singletons_mutex_);
				auto it = singletons_.find(type);
				if (it != singletons_.end())
				{
					delete instance;
					return static_cast<T*>(it->second);
				}
				singletons_[type] = instance;
				return instance;
			}
		}

	private:
		InitConfig ParseInitConfig(const std::string& arg);
		bool CreateServices(void* appHook, void* extraHook, char* pScmdline);
		bool ExecuteDefaultTask();

		template<typename T>
		T* GetSystemWithDependencies();
		void ResolveDependencies(const std::vector<std::type_index>& dependencies);

		// Platform-specific system creation helpers
		IWindowService* CreateWindowSystem(bool isHeadless);

		EngineImpl* m_pImpl;

		// Storage for singletons using raw pointers. Guarded by singletons_mutex_ —
		// Get<T>() is reachable from any thread, so the map must be thread-safe.
		std::unordered_map<std::type_index, void*> singletons_;
		mutable std::shared_mutex singletons_mutex_;
	};

	// Template implementation must be in header
	template<typename T>
	T* Engine::GetSystemWithDependencies()
	{
		// Special handling for WindowSystem - redirect to public method
		if constexpr (std::is_same_v<T, IWindowService>) {
			return reinterpret_cast<T*>(getWindowService());
		}
		else if constexpr (std::is_abstract_v<T>) {
			return nullptr;
		}
		else {
			// Handle regular IService classes — same locked find-or-create as Get<>.
			auto type = std::type_index(typeid(T));
			{
				std::shared_lock<std::shared_mutex> lock(singletons_mutex_);
				auto it = singletons_.find(type);
				if (it != singletons_.end())
					return static_cast<T*>(it->second);
			}

			T* instance = new T();
			if (!instance)
				return nullptr;

			{
				std::unique_lock<std::shared_mutex> lock(singletons_mutex_);
				// Another thread may have raced us to insert the same type.
				auto it = singletons_.find(type);
				if (it != singletons_.end())
				{
					delete instance;
					return static_cast<T*>(it->second);
				}
				singletons_[type] = instance;
			}

			// ResolveDependencies re-enters Get<>, so it must run outside the unique lock.
			auto dependencies = instance->GetDependencies();
			ResolveDependencies(dependencies);
			return instance;
		}
	}

	extern Engine* g_Engine;
}