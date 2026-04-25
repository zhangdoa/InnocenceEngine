#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class GIDenoisePass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(GIDenoisePass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		// GI history (CL1 storage convention: rgb = sample-count-weighted
		// radiance Σ Lᵢ, a = sample count N). The final divide rgb/N is
		// owned by the spatial filter pass — for the GIATrous{1,2,4}
		// cascade in CL1 use GetIrradianceForFilter() instead.
		TextureComponent* GetCurrentResult();

		// Per-pixel temporal-variance moments (r=E[L], g=E[L²], b=N).
		// CL2 deletes both this and the cascade that consumes it; kept
		// in CL1 for compatibility with GIATrous{1,2,4}.
		TextureComponent* GetCurrentMoments();

		// Normalised irradiance (rgb = lighting/N, a = N) for the
		// GIATrous{1,2,4} cascade to consume in CL1. Transitional surface
		// — CL2 deletes it once the variable-radius blur reads
		// sample-count-weighted history directly.
		TextureComponent* GetIrradianceForFilter();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;

		// Ping-pong full-screen GI irradiance history. Role (current /
		// previous) swaps every frame based on frame count parity.
		TextureComponent* m_GIHistory_Even;
		TextureComponent* m_GIHistory_Odd;

		// Ping-pong full-screen SVGF moments history.
		TextureComponent* m_Moments_Even;
		TextureComponent* m_Moments_Odd;

		// Ping-pong previous-frame world-space position. Replaces the
		// linear-depth-in-history-alpha source we lose when GIHistory.a
		// becomes the sample count.
		TextureComponent* m_PrevWorldPos_Even;
		TextureComponent* m_PrevWorldPos_Odd;

		// Ping-pong smoothed colour-delta (r = lumaA − lumaB EMA at 1/8).
		// Drives the dynamic history cap.
		TextureComponent* m_ColorDelta_Even;
		TextureComponent* m_ColorDelta_Odd;

		// Single-buffer scratch surface for the cascade input. Not
		// ping-ponged — written every frame, read by GIATrous1Pass within
		// the same frame.
		TextureComponent* m_IrradianceForFilter;

		TextureComponent* GetPreviousResult();
		TextureComponent* GetPreviousMoments();
		TextureComponent* GetCurrentPrevWorldPos();
		TextureComponent* GetPreviousPrevWorldPos();
		TextureComponent* GetCurrentColorDelta();
		TextureComponent* GetPreviousColorDelta();

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
