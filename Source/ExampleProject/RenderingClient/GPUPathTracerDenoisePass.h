#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// TASK-77.1.2 — denoise read + composition for the GPU path tracer.
	// One thread per pixel: reads the per-frame noisy radiance plus the
	// per-pixel primary-hit data produced by GPUPathTracerPass, looks up
	// the world-space hash-grid cell the writer populated at primary hit,
	// and emits denoised = lerp(noisy, cached, saturate(sampleCount/32)).
	//
	// Routing: PT-primary mode only. ExampleRenderingClient routes this
	// pass's result into l_hdrSource when m_GPUPathTracerActive is true.
	// When PT-primary is off the pass is bypassed via m_Bypassed; the
	// ClearOnBypass override clears the denoised UAV so debug A/B doesn't
	// see stale output.
	class GPUPathTracerDenoisePass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GPUPathTracerDenoisePass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		// TASK-182 ClearOnBypass: when PT-primary toggles off the pass is
		// bypassed; this override clears m_Result so the consumer site at
		// ExampleRenderingClient.cpp:483-490 doesn't surface a stale frame
		// after the rasterizer fork takes over.
		bool RecordClearCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		GPUResourceComponent* GetResult();

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		RenderPassComponent*    m_RenderPassComp    = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;
		TextureComponent*       m_Result            = nullptr;

		void CreateResult();
		void OnResize();
	};
} // namespace Inno
