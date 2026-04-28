#pragma once
#include <vector>
#include "../../Engine/Interface/IRenderingClient.h"

namespace Inno
{
	class IRenderPass;
	class ExampleRenderingClientImpl;
	class ExampleRenderingClient : public IRenderingClient
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(ExampleRenderingClient);

		// Inherited via IRenderingClient
		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool PrepareCommands() override;
		bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) override;
		bool FinalizeGPUResults() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		// Snapshot of passes the client owns, in the order PrepareCommands would
		// dispatch them. Call from the main thread between frames; returned
		// pointers are stable for the process lifetime.
		std::vector<IRenderPass*> GetDispatchedPasses() const;

	private:
		ExampleRenderingClientImpl* m_Impl;
	};
}