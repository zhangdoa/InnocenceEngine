#pragma once
#include "Common/ClassTemplate.h"
#include "Common/LogService.h"
#include "Interface/IService.h"
#include "Interface/IRenderingClient.h"
#include "Interface/ILogicClient.h"
#include <type_traits>

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
		const FixedSizeString<128>& GetApplicationName();
		IWindowService* getWindowService();
		float getTickTime();

		template <typename T>
		T* Get() 
		{
			auto type = std::type_index(typeid(T));
			auto it = singletons_.find(type);
			if (it == singletons_.end()) 
			{
				// For IService classes, use dependency resolution
				if constexpr (std::is_base_of_v<IService, T>) {
					return GetSystemWithDependencies<T>();
				}
				else {
					// Essential Services: Create directly
					T* instance = new T();
					singletons_[type] = instance;
					return instance;
				}
			}
			return static_cast<T*>(singletons_[type]);
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

		// Storage for singletons using raw pointers
		std::unordered_map<std::type_index, void*> singletons_;
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
			// Handle regular IService classes
			auto type = std::type_index(typeid(T));
			auto it = singletons_.find(type);
			if (it == singletons_.end()) {
				// Create the type directly
				T* instance = new T();
				if (!instance) {
					return nullptr;
				}
				singletons_[type] = instance;
				// Resolve dependencies after creation
				auto dependencies = instance->GetDependencies();
				ResolveDependencies(dependencies);
				return instance;
			}
			return static_cast<T*>(it->second);
		}
	}

	extern Engine* g_Engine;
}