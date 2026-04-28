#pragma once
#include <atomic>
#include "IService.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/GPUResourceComponent.h"
#include "../Component/SamplerComponent.h"
#include "../Component/GPUBufferComponent.h"
#include "../Common/Math.h"

namespace Inno
{
	class IRenderingContext {};

	class IRenderPass : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderPass);

        virtual bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) = 0;
		virtual RenderPassComponent* GetRenderPassComp() = 0;
		CommandListComponent* GetCommandListComp(GPUEngineType gpuEngineType)
		{
			switch (gpuEngineType)
			{
			case GPUEngineType::Graphics:
				return m_CommandListComp_Graphics;
			case GPUEngineType::Compute:
				return m_CommandListComp_Compute;
			case GPUEngineType::Copy:
				return m_CommandListComp_Copy;
			default:
				return nullptr;
			}
		}

		// Runtime bypass toggle — when true the dispatch site skips both
		// PrepareCommandList (no recording) AND the matching Execute / SignalOnGPU
		// pair in ExecuteCommands (no submission, no fence advance). Downstream
		// consumers that WaitOnGPU on a bypassed pass also skip the wait, so the
		// CL lifecycle stays consistent: a bypassed pass's command list is neither
		// Reset nor Closed nor submitted. Resources keep last-frame contents (any
		// RTV clear inside PrepareCommandList is also skipped); downstream consumers
		// must tolerate stale reads. Written from the editor IPC worker thread, read
		// from the render thread — atomic with relaxed ordering is the contract.
		std::atomic<bool> m_Bypassed { false };

		// Render-thread-only mirror used to detect ON↔OFF transitions for edge-triggered
		// logging. Never read or written by any other thread.
		bool m_BypassedPrev { false };

	protected:
		CommandListComponent* m_CommandListComp_Graphics;
		CommandListComponent* m_CommandListComp_Compute;
		CommandListComponent* m_CommandListComp_Copy;
	};
}