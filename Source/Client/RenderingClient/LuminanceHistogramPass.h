#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{	
	class LuminanceHistogramPassRenderingContext : public IRenderingContext
	{
		public:
		GPUResourceComponent *m_input;
	};

	class LuminanceHistogramPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(LuminanceHistogramPass)

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
		ShaderProgramComponent *m_SPC;
		SamplerComponent *m_SamplerComp;	
		TextureComponent* m_TextureComp;

		GPUBufferComponent *m_luminanceHistogram = 0;
	};
} // namespace Inno
