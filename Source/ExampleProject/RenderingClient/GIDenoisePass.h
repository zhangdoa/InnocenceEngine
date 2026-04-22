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

		// Denoised per-pixel indirect irradiance for the current frame.
		// rgb = irradiance, a = linear depth of the pixel that produced the
		// sample (next-frame reprojection validity).
		TextureComponent* GetCurrentResult();

		// Per-pixel temporal-variance moments for the current frame.
		// r = E[luma], g = E[luma²], b = history-count N (frames this pixel
		// has reprojected without disocclusion), a = unused. Temporal
		// variance = g − r² — used here to drive blend rate and exposed for
		// the [I.3e.3] A-trous pass to use as the edge-stopping luminance
		// weight.
		TextureComponent* GetCurrentMoments();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;

		// Ping-pong full-screen GI irradiance history. Role (current /
		// previous) swaps every frame based on frame count parity.
		TextureComponent* m_GIHistory_Even;
		TextureComponent* m_GIHistory_Odd;

		// Ping-pong full-screen SVGF moments history (see GetCurrentMoments).
		TextureComponent* m_Moments_Even;
		TextureComponent* m_Moments_Odd;

		TextureComponent* GetPreviousResult();
		TextureComponent* GetPreviousMoments();

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
