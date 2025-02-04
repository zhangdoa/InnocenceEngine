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
		ShaderProgramComponent *m_ShaderProgramComp;
		SamplerComponent *m_SamplerComp;
		SamplerComponent *m_SamplerComp_RandomRot;

		uint32_t m_kernelSize = 64;
		std::vector<Math::Vec4> m_Kernel;
		std::vector<Math::Vec4> m_Noise;

		GPUBufferComponent *m_KernelGPUBuffer;
		TextureComponent *m_NoiseTexture;
		TextureComponent* m_Result;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
