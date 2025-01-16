#pragma once
#include "Common/ClassTemplate.h"
#include "Common/Logger.h"

#include "Interface/IWindowSystem.h"
#include "RenderingServer/IRenderingServer.h"

#include "Interface/IRenderingClient.h"
#include "Interface/ILogicClient.h"

#include "../Client/ClientMetadata.h"

#define PPCAT_NX(A, B) A ## B
#define PPCAT(A, B) PPCAT_NX(A, B)
#define STRINGIZE_NX(A) #A
#define STRINGIZE(A) STRINGIZE_NX(A)

#define INNO_RENDERING_CLIENT_HEADER_PATH ../Client/RenderingClient/INNO_RENDERING_CLIENT.h
#define INNO_LOGIC_CLIENT_HEADER_PATH ../Client/LogicClient/INNO_LOGIC_CLIENT.h

#include STRINGIZE(INNO_RENDERING_CLIENT_HEADER_PATH)
#include STRINGIZE(INNO_LOGIC_CLIENT_HEADER_PATH)

namespace Inno
{
	enum EngineMode { Host, Slave };

	enum RenderingServer { GL, DX11, DX12, VK, MT };

	struct InitConfig
	{
		EngineMode engineMode = EngineMode::Host;
		RenderingServer renderingServer = RenderingServer::GL;
		LogLevel logLevel = LogLevel::Success;
	};

	class EngineImpl;
	class Engine
	{
	public:
		Engine();
		~Engine();

		bool Setup(			
			// Windows: For hInstance
			// macOS: For window bridge
			void* appHook,
			// Windows: For hwnd
			// macOS: For Metal rendering backend bridge
			void* extraHook,
			char* pScmdline);
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
				// Create and store the singleton if it doesn't exist
				T* instance = new T();
				singletons_[type] = instance;
				return instance;
			}
			return static_cast<T*>(singletons_[type]);
		}

	private:
		InitConfig ParseInitConfig(const std::string& arg);
		bool CreateServices(void* appHook, void* extraHook, char* pScmdline);
		bool ExecuteDefaultTask();

		EngineImpl* m_pImpl;

		// Storage for singletons using raw pointers
		std::unordered_map<std::type_index, void*> singletons_;
	};

	extern Engine* g_Engine;
}