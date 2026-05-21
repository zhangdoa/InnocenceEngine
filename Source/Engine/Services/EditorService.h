#pragma once
#include "../Interface/IService.h"

#include <functional>
#include <memory>
#include <string>

namespace Inno
{
	// PIMPL keeps the WebSocket / nlohmann::json types out of every consumer of this header.
	struct EditorServiceImpl;

	class EditorService : public IService
	{
	public:
		EditorService();
		~EditorService(); // out-of-line — m_Impl PIMPL needs the full type for destruction
		EditorService(const EditorService&) = delete;
		EditorService& operator=(const EditorService&) = delete;

		bool Setup(IServiceConfig* config) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		// Broadcasts SCREENSHOT_SAVED to every connected editor client. Caller-synchronised;
		// invoked from arbitrary threads (matches BroadcastSceneUpdated).
		bool BroadcastScreenshotSaved(bool in_Ok, const std::string& in_AbsolutePath, const std::string& in_ErrorReason);

	private:
		void RegisterBuiltinHandlers();
		void RegisterIntrospectionHandlers();
		void RegisterDevAndSceneHandlers();
		void RegisterEntityHandlers();
		void BroadcastSceneUpdated();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		void* m_Server = nullptr; // Opaque pointer to ix::WebSocketServer
		std::unique_ptr<EditorServiceImpl> m_Impl;

		// SceneService stores a raw pointer to this functor — must out-live the service.
		std::function<void()> m_sceneLoadedCallback;
	};
}
