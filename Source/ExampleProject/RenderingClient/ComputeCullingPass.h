#pragma once
#include "../../Engine/Interface/IRenderPass.h"

namespace Inno
{
	// Shared implementation for "compute culling pass that writes an indirect
	// draw-command buffer": per-frame CB in, GPUModelData + Material SRVs in,
	// Dispatch, RWStructuredBuffer<DX12IndirectDrawCommand> out. Subclasses
	// only supply an identity (name prefix + compute shader path); the full
	// resource binding layout and command-list lifecycle live here so a fix
	// applied once (TASK-44's SetCurrentState, TASK-61's clip-space frustum
	// test, etc.) can't drift between siblings.
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
		// Subclass identity. Return pointer-to-static-literal; lifetime must
		// outlive the pass (simplest: return a string literal).
		virtual const char* GetPassName() const = 0;
		virtual const char* GetComputeShaderPath() const = 0;

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		RenderPassComponent* m_RenderPassComp = nullptr;
		ShaderProgramComponent* m_ShaderProgramComp = nullptr;
		GPUBufferComponent* m_IndirectDrawCommandBuffer = nullptr;
	};
}
