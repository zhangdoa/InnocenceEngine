#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class SSRCReprojectionPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SSRCReprojectionPass)

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		TextureComponent* GetCurrentFrameResult();
		TextureComponent* GetPreviousFrameResult();
		TextureComponent* GetCurrentProbePosition();
		TextureComponent* GetPreviousProbePosition();
		TextureComponent* GetCurrentProbeNormal();
		TextureComponent* GetPreviousProbeNormal();
		TextureComponent* GetProbeMask();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent* m_RenderPassComp;
		ShaderProgramComponent* m_ShaderProgramComp;
		TextureComponent* m_SSRC_Odd;
		TextureComponent* m_SSRC_Even;
		TextureComponent* m_ProbePosition_Odd;
		TextureComponent* m_ProbePosition_Even;
		TextureComponent* m_ProbeNormal_Odd;
		TextureComponent* m_ProbeNormal_Even;
		TextureComponent* m_ProbeMask;

		ShaderStage m_ShaderStage;

		bool RenderTargetsCreationFunc();

	};
} // namespace Inno
