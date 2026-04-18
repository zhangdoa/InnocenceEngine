#pragma once
#include "IService.h"

namespace Inno
{
	class IRenderingConfig {};
	class IRenderingClient : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingClient);

		virtual bool PrepareCommands() { return true; }
		virtual bool ExecuteCommands(IRenderingConfig* renderingConfig = nullptr) = 0;
		virtual bool GetValidationPassed() const { return true; }

		// Called once during Engine::Terminate, AFTER WaitForGPUIdle and BEFORE
		// any CPU-heavy LogicClient shutdown work (e.g. the CPU path tracer).
		// The GPU is guaranteed alive here; this is the only legal place for
		// GPU-dependent finalization (readback, final flush). Overrides must
		// NOT perform long-running CPU work — that belongs in Terminate, which
		// runs after this phase and may race the GPU TDR window.
		virtual bool FinalizeGPUResults() { return true; }
	};
}