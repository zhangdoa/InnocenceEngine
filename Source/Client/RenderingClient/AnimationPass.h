#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class AnimationPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(AnimationPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		SamplerComponent *m_SamplerComp;

		bool RenderTargetsReservationFunc();
		bool RenderTargetsCreationFunc();
		bool DepthStencilRenderTargetsCreationFunc();
	};
} // namespace Inno
