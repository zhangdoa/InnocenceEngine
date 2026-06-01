#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// Compute-culling base: per-frame CB + GPUModelData/Material SRVs in,
	// RWStructuredBuffer<DX12IndirectDrawCommand> out. Subclasses supply only name + shader path.
	class ComputeCullingPass : public IRenderPass
	{
	public:
		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		bool PrepareCommandList(IRenderingContext* renderingContext = nullptr) override;
		RenderPassComponent* GetRenderPassComp() override;

		GPUResourceComponent* GetResult();

	protected:
		// Returned pointer must outlive the pass — simplest is a string literal.
		virtual const char* GetPassName() const = 0;
		virtual const char* GetComputeShaderPath() const = 0;

		bool SetupFromRenderGraph();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		RenderPassComponent* m_RenderPassComp = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;
		GPUBufferComponent* m_IndirectDrawCommandBuffer = nullptr;
	};
}
