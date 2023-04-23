#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{	
	class TransparentBlendPassRenderingContext : public IRenderingContext
	{
		public:
		GPUResourceComponent *m_output;
	};

	class TransparentBlendPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(TransparentBlendPass)

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
	};
} // namespace Inno
