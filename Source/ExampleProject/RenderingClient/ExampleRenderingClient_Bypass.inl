// Anonymous namespace inside `namespace Inno {}` — TU-local linkage prevents header leakage.

namespace
{
	// Edge-triggered ON/OFF logging; m_BypassedPrev is render-thread-only.
	// m_ClearOnBypass routes to RecordClearCommandList (Execute + Signal still fire so downstream
	// WaitOnGPU drains); plain bypass is full-skip.
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

	// True only for the full-skip case. A clear-on-bypass pass is treated as live for both
	// Execute gating and WaitIfActive draining (its CL was Recorded + Executed + Signalled, so
	// skipping the matching Execute here would crash DX12 — allocator already rotated).
	inline bool IsBypassed(IRenderPass& pass)
	{
		return pass.m_Bypassed.load(std::memory_order_relaxed) && !pass.m_ClearOnBypass;
	}

	// Waiting on a bypassed/unactivated producer would block forever — no Signal is emitted on
	// its renderpass this frame.
	inline void WaitIfActive(IRenderPass& pass, GPUEngineType in_QueueType, GPUEngineType in_SemaphoreType)
	{
		if (pass.GetStatus() != ObjectStatus::Activated)
			return;
		if (IsBypassed(pass))
			return;
		g_Engine->Get<GraphicsHardwareService>()->WaitOnGPU(pass.GetRenderPassComp(), in_QueueType, in_SemaphoreType);
	}
}
