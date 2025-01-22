#pragma once
#include "../IRenderingServer.h"

#include "../../Common/ObjectPool.h"

#include "../../Component/DX12MeshComponent.h"
#include "../../Component/DX12TextureComponent.h"
#include "../../Component/DX12MaterialComponent.h"
#include "../../Component/DX12RenderPassComponent.h"
#include "../../Component/DX12ShaderProgramComponent.h"
#include "../../Component/DX12SamplerComponent.h"
#include "../../Component/DX12GPUBufferComponent.h"

namespace Inno
{
	class DX12RenderingComponentPool : public IRenderingComponentPool
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12RenderingComponentPool);
		
		// Inherited via IRenderingComponentPool
		void Initialize() override;
		void Terminate() override;

		MeshComponent* AddMeshComponent(const char* name) override;
		TextureComponent* AddTextureComponent(const char* name) override;
		MaterialComponent* AddMaterialComponent(const char* name) override;
		RenderPassComponent* AddRenderPassComponent(const char* name) override;
		ShaderProgramComponent* AddShaderProgramComponent(const char* name) override;
		SamplerComponent* AddSamplerComponent(const char* name = "") override;
		GPUBufferComponent* AddGPUBufferComponent(const char* name) override;
        IPipelineStateObject* AddPipelineStateObject() override;
        ICommandList* AddCommandList() override;
        ISemaphore* AddSemaphore() override;

		bool Delete(MeshComponent* rhs) override;
		bool Delete(TextureComponent* rhs) override;
		bool Delete(MaterialComponent* rhs) override;
		bool Delete(RenderPassComponent* rhs) override;
		bool Delete(ShaderProgramComponent* rhs) override;
		bool Delete(SamplerComponent* rhs) override;
		bool Delete(GPUBufferComponent* rhs) override;
		bool Delete(IPipelineStateObject* rhs) override;
		bool Delete(ICommandList* rhs) override;
		bool Delete(ISemaphore* rhs) override;

	private:
	    TObjectPool<DX12MeshComponent>* m_MeshComponentPool = 0;
        TObjectPool<DX12MaterialComponent>* m_MaterialComponentPool = 0;
        TObjectPool<DX12TextureComponent>* m_TextureComponentPool = 0;
        TObjectPool<DX12RenderPassComponent>* m_RenderPassComponentPool = 0;
        TObjectPool<DX12ShaderProgramComponent>* m_ShaderProgramComponentPool = 0;
        TObjectPool<DX12SamplerComponent>* m_SamplerComponentPool = 0;
        TObjectPool<DX12GPUBufferComponent>* m_GPUBufferComponentPool = 0;
        TObjectPool<DX12PipelineStateObject>* m_PSOPool = 0;
        TObjectPool<DX12CommandList>* m_CommandListPool = 0;
        TObjectPool<DX12Semaphore>* m_SemaphorePool = 0;
	};
}