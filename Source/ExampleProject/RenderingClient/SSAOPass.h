#pragma once
#include "../../Engine/Common/Array.h"
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	class SSAOPass : public IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(SSAOPass)

		bool Setup(IServiceConfig *systemConfig = nullptr) override;
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

		// Mirrors the canonical kernel count in SSAONoisePass.comp (sampleCount).
		uint32_t m_kernelSize = 32;
		Inno::Array<Math::Vec4> m_Kernel;
		Inno::Array<Math::Vec4> m_Noise;

		GPUBufferComponent *m_KernelGPUBuffer;
		TextureComponent *m_NoiseTexture;
		TextureComponent* m_Result;

		bool RenderTargetsCreationFunc();
	};
} // namespace Inno
