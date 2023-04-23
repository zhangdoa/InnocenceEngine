#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{	
	class VXGIRayTracingPassRenderingContext : public IRenderingContext
	{
		public:
		GPUResourceComponent *m_input;
		GPUResourceComponent *m_output;
	};

	class VXGIRayTracingPass : IRenderPass
	{
	public:
		INNO_CLASS_SINGLETON(VXGIRayTracingPass)

		bool Setup(ISystemConfig *systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext *renderingContext) override;
		RenderPassComponent *GetRenderPassComp() override;

		GPUResourceComponent *GetResult();

	private:
		ObjectStatus m_ObjectStatus;
		RenderPassComponent *m_RenderPassComp;
		ShaderProgramComponent *m_ShaderProgramComp;
		SamplerComponent *m_SamplerComp;
		TextureComponent *m_TextureComp;

		GPUBufferComponent *m_RaySBufferGPUBufferComp;
		GPUBufferComponent *m_ProbeIndexSBufferGPUBufferComp;

		std::vector<Math::Vec4> m_Ray;
		std::vector<Math::TVec4<uint32_t>> m_ProbeIndex;
		std::default_random_engine m_generator;
		std::uniform_int_distribution<uint32_t> m_randomInt;
	};
} // namespace Inno
