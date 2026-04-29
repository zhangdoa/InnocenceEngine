// Bypass-aware dispatch / wait helpers for the example rendering client.
// Included once at translation-unit scope inside ExampleRenderingClient.cpp,
// nested under `namespace Inno { ... }`. Anonymous-namespace TU-local linkage
// keeps the helpers from leaking via header inclusion.

namespace
{
	// Single dispatch-site honour for IRenderPass::m_Bypassed. One relaxed
	// atomic load per pass per frame is the only added cost when nothing is
	// bypassed. Edge-triggered logging fires once on each ON↔OFF transition;
	// the previous-state mirror is render-thread-only so it stays a plain bool.
	//
	// TASK-182: when m_Bypassed && m_ClearOnBypass the dispatch site routes
	// to RecordClearCommandList instead of full skip — the pass records a
	// CL that clears its UAV outputs to neutral values (typically zero), and
	// the matching Execute / SignalOnGPU still fires so downstream WaitOnGPU
	// drains as in the live path. Plain bypass (m_ClearOnBypass=false) keeps
	// TASK-171 semantics: full skip, no Execute, no Signal.
	template<typename PassT, typename... Args>
	inline void DispatchOrBypass(PassT& pass, Args&&... args)
	{
		const bool l_bypass = pass.m_Bypassed.load(std::memory_order_relaxed);
		if (l_bypass != pass.m_BypassedPrev)
		{
			pass.m_BypassedPrev = l_bypass;
			auto* l_comp = pass.GetRenderPassComp();
			const char* l_name = l_comp ? l_comp->m_InstanceName.c_str() : "<unknown>";
			Log(Verbose, "RenderPass: ", l_name, " bypass = ", l_bypass ? "ON" : "OFF",
				(l_bypass && pass.m_ClearOnBypass) ? " (clear-on-bypass)" : "");
		}
		if (l_bypass)
		{
			if (pass.m_ClearOnBypass)
				pass.RecordClearCommandList();
			return;
		}
		pass.PrepareCommandList(std::forward<Args>(args)...);
	}

	// Execute-side mirror of DispatchOrBypass. PrepareCommandList drives the
	// per-frame Reset/Close cycle on the pass's command list; if that was
	// bypassed, the matching Execute / SignalOnGPU here MUST also be elided
	// or DX12 submits a CL whose allocator was rotated out from under it.
	// Logging stays in DispatchOrBypass — this side is silent (same atomic
	// value, the value cannot have changed mid-frame in any case the user
	// observes).
	//
	// TASK-182: a clear-on-bypass pass DOES record + Execute + Signal — its
	// downstream consumers see a defined neutral value rather than stale
	// content. So IsBypassed reports true only for the full-skip case
	// (m_Bypassed && !m_ClearOnBypass); a clearing pass is treated as live
	// for both Execute gating and WaitIfActive draining.
	inline bool IsBypassed(IRenderPass& pass)
	{
		return pass.m_Bypassed.load(std::memory_order_relaxed) && !pass.m_ClearOnBypass;
	}

	// Cross-pass fence wait, gated on the upstream pass being live. A wait on
	// a bypassed (or unactivated) producer would block forever — no Signal is
	// emitted on its renderpass this frame. Argument order mirrors the
	// underlying GraphicsHardwareService::WaitOnGPU(queueType, semaphoreType).
	inline void WaitIfActive(IRenderPass& pass, GPUEngineType in_QueueType, GPUEngineType in_SemaphoreType)
	{
		if (pass.GetStatus() != ObjectStatus::Activated)
			return;
		if (IsBypassed(pass))
			return;
		g_Engine->Get<GraphicsHardwareService>()->WaitOnGPU(pass.GetRenderPassComp(), in_QueueType, in_SemaphoreType);
	}
}
