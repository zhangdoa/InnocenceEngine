#pragma once
#include "../Interface/IService.h"

#include <functional>
#include <memory>
#include <string>

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

		// Broadcasts a SCREENSHOT_SAVED event to every connected editor client.
		// Called by the rendering client after AssetService::Save returns, so
		// the editor toast can name the absolute path on success or the
		// failure reason on error (TASK-211 AC-4). Thread-safety: caller-side;
		// matches BroadcastSceneUpdated's contract (the underlying ixwebsocket
		// send is invoked from arbitrary threads in the existing handler path).
		// Returns true iff the WS server is up and the event was queued to all
		// currently-connected clients without an exception.
		bool BroadcastScreenshotSaved(bool in_Ok, const std::string& in_AbsolutePath, const std::string& in_ErrorReason);

	private:
		void RegisterBuiltinHandlers();
		void BroadcastSceneUpdated();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		void* m_Server = nullptr; // Opaque pointer to ix::WebSocketServer
		std::unique_ptr<EditorServiceImpl> m_Impl;

		// SceneService::AddSceneLoadedCallback stores a raw function pointer,
		// so the functor must out-live the service.
		std::function<void()> m_sceneLoadedCallback;
	};
}
