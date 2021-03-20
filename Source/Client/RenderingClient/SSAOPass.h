#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class SSAOPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SSAOPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassDataComponent *GetRPDC() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassDataComponent *m_RPDC;
		ShaderProgramComponent *m_SPC;
		SamplerDataComponent *m_SDC;	
		TextureDataComponent* m_TDC;	
		SamplerDataComponent *m_SDC_RandomRot;

		uint32_t m_kernelSize = 64;
		std::vector<InnoMath::Vec4> m_SSAOKernel;
		std::vector<InnoMath::Vec4> m_SSAONoise;

		GPUBufferDataComponent *m_SSAOKernelGPUBuffer;
		TextureDataComponent *m_SSAONoiseTDC;
	};
} // namespace Inno
