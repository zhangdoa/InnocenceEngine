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
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_SPC;
		SamplerComponent *m_SamplerComp;	
		TextureComponent* m_TextureComp;	
		SamplerComponent *m_SamplerComp_RandomRot;

		uint32_t m_kernelSize = 64;
		std::vector<InnoMath::Vec4> m_SSAOKernel;
		std::vector<InnoMath::Vec4> m_SSAONoise;

		GPUBufferComponent *m_SSAOKernelGPUBuffer;
		TextureComponent *m_SSAONoiseTextureComp;
	};
} // namespace Inno
