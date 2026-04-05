#pragma once
#include "../RenderPassResourceService.h"
#include "../../Common/ObjectPool.h"
#include "DX12Headers.h"

namespace Inno
{
	struct DX12Context;

	class DX12RenderPassResourceService : public RenderPassResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12RenderPassResourceService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override;
		bool Terminate() override;

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

		IPipelineStateObject* AddPipelineStateObject() override;
		ISemaphore* AddSemaphore() override;
		bool Add(IOutputMergerTarget*& rhs) override;

		bool Delete(RenderPassComponent* ptr) override;
		bool Delete(IPipelineStateObject* rhs) override;
		bool Delete(ISemaphore* rhs) override;
		bool Delete(IOutputMergerTarget* rhs) override;

		Math::Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) override;

	protected:
		bool OnOutputMergerTargetsCreated(RenderPassComponent* renderPass) override;
		bool CreatePipelineStateObject(RenderPassComponent* renderPass) override;
		bool CreateFenceEvents(RenderPassComponent* renderPass) override;

	private:
		bool CreateRootSignature(RenderPassComponent* renderPassComp);
		bool CreateGraphicsPipelineStateObject(RenderPassComponent* renderPassComp, DX12PipelineStateObject* PSO);
		bool CreateRaytracingPipelineStateObject(RenderPassComponent* renderPassComp, DX12PipelineStateObject* PSO);
		D3D12_DESCRIPTOR_RANGE1 GetDescriptorRange(RenderPassComponent* renderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc);

		DX12Context* m_ctx = nullptr;

		TObjectPool<DX12PipelineStateObject>* m_PSOPool = nullptr;
		TObjectPool<DX12Semaphore>* m_SemaphorePool = nullptr;
		TObjectPool<DX12OutputMergerTarget>* m_OutputMergerTargetPool = nullptr;
	};
}
