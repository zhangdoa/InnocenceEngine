#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class BRDFLUTPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(BRDFLUTPass)

		bool Setup(IServiceConfig *systemConfig = nullptr) override;
        bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		bool SetupFromRenderGraph();

		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		SamplerComponent *m_SamplerComp;
		TextureComponent *m_Result;
	};
} // namespace Inno
