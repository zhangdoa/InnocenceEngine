#pragma once

#include "../Interface/IService.h"
#include <memory>

namespace ix { class WebSocketServer; }

namespace Inno
{
	class EditorService : public IService
	{
	public:
		EditorService();
		~EditorService();
		EditorService(const EditorService& rhs) = delete;
		EditorService& operator=(const EditorService& rhs) = delete;
		EditorService(EditorService&& other) = default;
		EditorService& operator=(EditorService&& other) = default;

		bool Setup(IServiceConfig* config) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		void NotifyViewportReady(void* sharedHandle);

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		std::unique_ptr<ix::WebSocketServer> m_Server;
		uint32_t m_clientPID = 0;
	};
}
