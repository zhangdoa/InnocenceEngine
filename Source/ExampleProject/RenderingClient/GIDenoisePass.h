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

		// GI history (sample-count-weighted: rgb = Σ Lᵢ, a = sample
		// count N). The horizontal filter pass consumes this directly
		// and re-emits it after one separable blur axis; the vertical
		// pass divides out N to produce the final irradiance.
		TextureComponent* GetCurrentResult();

		// Per-pixel temporal-variance moments (r=E[L], g=E[L²], b=N).
		// Dead under the §2.4.3 paper-faithful filter — kept on this CL
		// to keep the shader binding-set diff small; the moments
		// ping-pong becomes a tracked follow-up (TASK-129).
		TextureComponent* GetCurrentMoments();

		// Per-pixel blur mask consumed by GIFilterHorizontal/Vertical.
		// Stored normalised in [0, 1] (sky encoded as -1/MaxBlurMask),
		// scalar R Float16. Capsaicin gi_denoiser.hlsl:32.
		TextureComponent* GetBlurMask();

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

		// Per-pixel blur mask. Single-buffered scalar Float16 — produced
		// by this pass and consumed entirely within the same frame by
		// GIFilterHorizontalPass and GIFilterVerticalPass.
		TextureComponent* m_BlurMask;

		TextureComponent* GetPreviousResult();
		TextureComponent* GetPreviousMoments();
		TextureComponent* GetCurrentPrevWorldPos();
		TextureComponent* GetPreviousPrevWorldPos();
		TextureComponent* GetCurrentColorDelta();
		TextureComponent* GetPreviousColorDelta();

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
