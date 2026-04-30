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
		//
		// Cross-queue exit-barrier caveat (TASK-161 / TASK-173): bypassing a pass
		// elides every barrier the pass would have emitted, including any
		// m_PostCLState = CrossQueueExit::ToCommon transition that hands a resource
		// off to a consumer on a different queue. This is GPU-safe under DX12
		// implicit-promotion-from-COMMON for the typical pattern: the producing pass
		// has run at least once before being bypassed (resource sits in COMMON from
		// the prior frame's exit barrier), and even first-frame bypass is safe
		// because INITIAL_STATE is implicitly COMMON. The contract a future pass
		// owner must preserve: if a CrossQueueExit-flagged pass adopts a non-COMMON
		// exit state, bypassing it would leave the resource in an indeterminate
		// state for cross-queue consumers — re-evaluate the bypass safety predicate
		// (or gate the bypass) before introducing such a pass.
		std::atomic<bool> m_Bypassed { false };

		// Render-thread-only mirror used to detect ON↔OFF transitions for edge-triggered
		// logging. Never read or written by any other thread.
		bool m_BypassedPrev { false };

		// TASK-182: when true and m_Bypassed is also true, the dispatch site calls
		// RecordClearCommandList instead of PrepareCommandList — the pass's UAV
		// outputs are cleared and the resulting CL still Executes / Signals so
		// downstream consumers read defined zero values instead of stale last-frame
		// content. When false (default) the bypass is a full skip per TASK-171.
		// Set in the owning pass's Setup(); the override that records the actual
		// clear commands is RecordClearCommandList() below.
		bool m_ClearOnBypass { false };

		// Pass-specific clear-on-bypass hook. Default no-op for passes that
		// either own no UAV outputs or have nothing downstream that requires a
		// neutral value when bypassed. Passes whose downstream consumers must
		// see zero (instead of last-frame stale) override this to record
		// transition + UAV-clear + transition-back commands on their compute
		// command list, matching the CL lifecycle PrepareCommandList drives so
		// the dispatch site can Execute / Signal uniformly. Called only when
		// m_Bypassed && m_ClearOnBypass.
		virtual bool RecordClearCommandList(IRenderingContext* renderingContext = nullptr) { return true; }

	protected:
		CommandListComponent* m_CommandListComp_Graphics;
		CommandListComponent* m_CommandListComp_Compute;
		CommandListComponent* m_CommandListComp_Copy;
	};
}