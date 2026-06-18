#pragma once
#include "Common/ClassTemplate.h"
#include "Common/Array.h"
#include "Common/LogService.h"
#include "Common/HashMap.h"
#include "Interface/IService.h"
#include "Interface/IRenderingClient.h"
#include "Interface/ILogicClient.h"
#include <type_traits>
#include <shared_mutex>

namespace Inno
{
	enum EngineMode { Host, Slave, Sidecar };

	enum GraphicsService { DX12, VK, MT };


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

		// Frame-lifecycle authority requests a clean shutdown (frame budget
		// reached, window closed, fatal GPU error). The main loop and the
		// recurrent rendering task observe it and unwind into Terminate() —
		// the one mode-independent stop, replacing per-service std::exit.
		void RequestShutdown();
		bool IsShutdownRequested() const;


		const FixedSizeString<128>& GetApplicationName();
		IWindowService* getWindowService();
		float getTickTime();

		template <typename T>
		T* Get()
		{
			auto type = std::type_index(typeid(T));
			{
				std::shared_lock<std::shared_mutex> lock(singletons_mutex_);
				auto it = singletons_.find(type);
				if (it != singletons_.end())
					return static_cast<T*>(it->second);
			}

			// new T() must run outside the unique lock: service constructors call
			// Log() which re-enters Get<>, and MSVC SRW-backed std::shared_mutex
			// forbids re-entry from unique into shared on the same thread. The
			// losing race on insert is handled by delete below.
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
		bool CreateServices(void* appHook, void* extraHook, char* pScmdline);
		bool ExecuteDefaultTask();
		void WireRenderingCallbacks();

		template<typename T>
		T* GetSystemWithDependencies();
		void ResolveDependencies(const Inno::Array<std::type_index>& dependencies);

		IWindowService* CreateWindowSystem(bool isHeadless);

		EngineImpl* m_pImpl;

		// Get<T>() is reachable from any thread; singletons_mutex_ guards both map and entries.
		Inno::HashMap<std::type_index, void*> singletons_;
		mutable std::shared_mutex singletons_mutex_;
	};

	template<typename T>
	T* Engine::GetSystemWithDependencies()
	{
		if constexpr (std::is_same_v<T, IWindowService>) {
			return reinterpret_cast<T*>(getWindowService());
		}
		else if constexpr (std::is_abstract_v<T>) {
			return nullptr;
		}
		else {
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