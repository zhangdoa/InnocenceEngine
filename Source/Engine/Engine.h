#pragma once
#include "Common/ClassTemplate.h"
#include "Common/LogService.h"
#include "Interface/ISystem.h"
#include "Interface/IRenderingClient.h"
#include "Interface/ILogicClient.h"
#include <type_traits>

namespace Inno
{
	enum EngineMode { Host, Slave };

	enum RenderingServer { DX12, VK, MT };

	struct InitConfig
	{
		EngineMode engineMode = EngineMode::Host;
		RenderingServer renderingServer = RenderingServer::DX12;
		LogLevel logLevel = LogLevel::Success;
		bool isHeadless = false;
		bool isOffscreen = false;
		char testCase[64] = {};
	};

	class IWindowSystem;
	class IRenderingServer;

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
		IRenderingServer* getRenderingServer();
		IWindowSystem* getWindowSystem();
		float getTickTime();

		template <typename T>
		T* Get() 
		{
			auto type = std::type_index(typeid(T));
			auto it = singletons_.find(type);
			if (it == singletons_.end()) 
			{
				// For ISystem classes, use dependency resolution
				if constexpr (std::is_base_of_v<ISystem, T>) {
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
		IWindowSystem* CreateWindowSystem(bool isHeadless);
		IRenderingServer* CreateRenderingServer(bool isHeadless, RenderingServer renderingServerType);

		EngineImpl* m_pImpl;

		// Storage for singletons using raw pointers
		std::unordered_map<std::type_index, void*> singletons_;
	};

	// Template implementation must be in header
	template<typename T>
	T* Engine::GetSystemWithDependencies()
	{
		// Special handling for WindowSystem and RenderingServer - redirect to public methods
		if constexpr (std::is_same_v<T, IWindowSystem>) {
			return reinterpret_cast<T*>(getWindowSystem());
		}
		else if constexpr (std::is_same_v<T, IRenderingServer>) {
			return reinterpret_cast<T*>(getRenderingServer());
		}
		else {
			// Handle regular ISystem classes
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