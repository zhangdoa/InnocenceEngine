#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class FinalBlendPassRenderingContext : public IRenderingContext
	{
		public:
		GPUResourceComponent *m_input;
	};

	class FinalBlendPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(FinalBlendPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
        bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;

		TextureComponent* m_Result;

		bool RenderTargetsCreationFunc();		
	};
} // namespace Inno
