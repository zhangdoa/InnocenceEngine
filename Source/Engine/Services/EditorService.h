#pragma once
#include "../Interface/IService.h"
#include <vector>
#include <string>

namespace Inno
{
	class EditorService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(EditorService);

		bool Setup(IServiceConfig* config) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		void NotifyViewportReady(void* sharedHandle);

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		void* m_Server = nullptr; // Opaque pointer to ix::WebSocketServer
		uint32_t m_clientPID = 0;
	};
}
