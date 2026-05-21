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

		// When true, the dispatch site skips both PrepareCommandList and the matching
		// Execute / SignalOnGPU pair, and downstream WaitOnGPU consumers also skip.
		// The CL is neither Reset nor Closed; resources keep last-frame contents
		// (any RTV clear inside PrepareCommandList is skipped). Written from the
		// editor IPC worker thread, read from the render thread.
		//
		// Safety constraint: bypass also elides any CrossQueueExit::ToCommon barrier
		// the pass would emit. Currently safe under DX12 implicit-promotion-from-
		// COMMON because INITIAL_STATE is COMMON and producing passes leave their
		// outputs there. A pass that adopts a non-COMMON cross-queue exit state
		// must gate its bypass predicate before this property holds.
		std::atomic<bool> m_Bypassed { false };

		// Render-thread-only mirror for edge-triggered ON↔OFF transition logging.
		bool m_BypassedPrev { false };

		// When true and m_Bypassed is also true, the dispatch site calls
		// RecordClearCommandList instead of skipping — the CL still Executes and
		// Signals so downstream consumers read defined zero values instead of
		// stale last-frame content.
		bool m_ClearOnBypass { false };

		// Default no-op; passes whose downstream consumers must see zero (not
		// last-frame stale) override to record transition + UAV-clear +
		// transition-back on their compute command list so the dispatch site
		// can Execute / Signal uniformly. Called only when m_Bypassed && m_ClearOnBypass.
		virtual bool RecordClearCommandList(IRenderingContext* renderingContext = nullptr) { return true; }

	protected:
		CommandListComponent* m_CommandListComp_Graphics;
		CommandListComponent* m_CommandListComp_Compute;
		CommandListComponent* m_CommandListComp_Copy;
	};
}