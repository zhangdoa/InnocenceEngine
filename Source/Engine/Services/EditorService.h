#pragma once
#include "../Interface/IService.h"

#include <functional>
#include <memory>

namespace Inno
{
	// The dispatcher table + handler types live in EditorService.cpp so the
	// WebSocket / nlohmann::json types do not bleed into every consumer of
	// this header. Forward-declare an opaque PIMPL.
	struct EditorServiceImpl;

	class EditorService : public IService
	{
	public:
		EditorService();
		~EditorService(); // out-of-line; m_Impl PIMPL needs full type for destruction
		EditorService(const EditorService&) = delete;
		EditorService& operator=(const EditorService&) = delete;

		bool Setup(IServiceConfig* config) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		void NotifyViewportReady(void* sharedHandle);

	private:
		void RegisterBuiltinHandlers();
		void BroadcastSceneUpdated();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		void* m_Server = nullptr; // Opaque pointer to ix::WebSocketServer
		uint32_t m_clientPID = 0;
		std::unique_ptr<EditorServiceImpl> m_Impl;

		// SceneService::AddSceneLoadedCallback stores a raw function pointer,
		// so the functor must out-live the service.
		std::function<void()> m_sceneLoadedCallback;
	};
}
