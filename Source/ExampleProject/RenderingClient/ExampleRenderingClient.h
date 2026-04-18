#pragma once
#include "../../Engine/Interface/IRenderingClient.h"

namespace Inno
{
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

	private:
		ExampleRenderingClientImpl* m_Impl;
	};
}