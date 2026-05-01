#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// EMA filter pass for the world-space hash-grid radiance cache.
	// Drains per-frame scratch contributions (written by GPUPathTracerPass
	// at primary hit) into the persistent value buffer that
	// GPUPathTracerDenoisePass reads. Mirrors Capsaicin's UpdateTilesMain
	// (gi1.comp:2160-2217) reduced to the flat-hash single-mip shape.
	//
	// Dispatch lifecycle: bypassed when GPUPathTracerPass is bypassed.
	// Sequenced after PT (waits on PT's compute Signal) and before the
	// denoise pass (denoise waits on this pass's Signal). One thread per
	// hash slot; HashGridCache::CELL_COUNT/64 thread groups.
	class GPUPathTracerHashGridFilterPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GPUPathTracerHashGridFilterPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

	private:
		ObjectStatus            m_ObjectStatus      = ObjectStatus::Terminated;
		RenderPassComponent*    m_RenderPassComp    = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;
	};
} // namespace Inno
