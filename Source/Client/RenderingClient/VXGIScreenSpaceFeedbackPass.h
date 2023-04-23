#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class VXGIScreenSpaceFeedbackPassRenderingContext : public IRenderingContext
	{
		public:
		GPUResourceComponent *m_output;
	};

	class VXGIScreenSpaceFeedbackPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(VXGIScreenSpaceFeedbackPass)

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
		TextureComponent *m_TextureComp;
	};
} // namespace Inno
