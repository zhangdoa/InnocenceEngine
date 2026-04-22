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

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;

		// Ping-pong full-screen GI irradiance history. Role (current /
		// previous) swaps every frame based on frame count parity.
		TextureComponent* m_GIHistory_Even;
		TextureComponent* m_GIHistory_Odd;

		TextureComponent* GetPreviousResult();

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
