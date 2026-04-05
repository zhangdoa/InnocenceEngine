#pragma once
#include "../GraphicsResourceService.h"
#include "DX12Context.h"
#include "../../Common/ObjectPool.h"

namespace Inno
{
	class DX12GraphicsResourceService : public GraphicsResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12GraphicsResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }
		DX12Context* GetDX12Context() { return m_ctx; }

		// DX12-specific pool types
		IPipelineStateObject* AddPipelineStateObject() override;
		ISemaphore* AddSemaphore() override;
		bool Add(IOutputMergerTarget*& rhs) override;

		// DX12-specific resource deletion
		bool Delete(MeshComponent* mesh) override;
		bool Delete(TextureComponent* texture) override;
		bool Delete(MaterialComponent* material) override;
		bool Delete(GPUBufferComponent* gpuBuffer) override;
		bool Delete(IPipelineStateObject* rhs) override;
		bool Delete(CommandListComponent* rhs) override;
		bool Delete(ISemaphore* rhs) override;
		bool Delete(IOutputMergerTarget* rhs) override;

		// Upload / transfer
		bool UploadToGPU(CommandListComponent* commandList, TextureComponent* texture) override;
		bool UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) override;
		bool Clear(CommandListComponent* commandList, TextureComponent* texture) override;
		bool Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) override;
		bool Copy(CommandListComponent* commandList, TextureComponent* src, TextureComponent* dst) override;
		bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList = nullptr) override;

		// Query
		std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) override;
		Vec4 ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) override;
		std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* textureComp) override;

	protected:
		// DX12-specific initialization
		bool InitializeImpl(MeshAssetHandle handle, std::vector<Vertex>& vertices, std::vector<Index>& indices) override;
		void ReleaseMeshGPUResourceImpl(MeshAssetHandle handle) override;
		bool InitializeImpl(TextureComponent* texture, void* textureData) override;
		bool InitializeImpl(ShaderProgramComponent* shaderProgram) override;
		bool InitializeImpl(SamplerComponent* sampler) override;
		bool InitializeImpl(GPUBufferComponent* gpuBuffer) override;
		bool InitializeImpl(EntityID entity) override;
		bool InitializeImpl(CommandListComponent* commandList) override;

		// Render pass initialization
		bool OnOutputMergerTargetsCreated(RenderPassComponent* renderPass) override;
		bool CreatePipelineStateObject(RenderPassComponent* renderPass) override;
		bool CreateFenceEvents(RenderPassComponent* renderPass) override;
		bool OnSceneLoadingStart() override;

		bool InitializePool() override;
		bool TerminatePool() override;

	public:
		// DX12-specific hardware-resource init called after DX12Context is ready
		bool CreateMipmapGenerator();
		bool CreateRaytracingResources();
		bool ReleaseRaytracingResources();
		bool ReleaseMipmapGenerator();

	private:
		// DX12-specific private helpers
		bool CreateSRV(TextureComponent* texture, uint32_t mostDetailedMip);
		bool CreateUAV(TextureComponent* texture, uint32_t mipSlice);
		bool CreateSRV(GPUBufferComponent* gpuBuffer);
		bool CreateUAV(GPUBufferComponent* gpuBuffer);
		bool CreateCBV(GPUBufferComponent* gpuBuffer);

		bool CreateRootSignature(RenderPassComponent* renderPassComp);
		bool CreateGraphicsPipelineStateObject(RenderPassComponent* renderPassComp, DX12PipelineStateObject* PSO);
		bool CreateRaytracingPipelineStateObject(RenderPassComponent* renderPassComp, DX12PipelineStateObject* PSO);
		D3D12_DESCRIPTOR_RANGE1 GetDescriptorRange(RenderPassComponent* renderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc);

		bool UploadToGPU(CommandListComponent* commandList, DX12MappedMemory* mappedMemory, DX12DeviceMemory* deviceMemory, GPUBufferComponent* gpuBuffer);

		template <typename U, typename T>
		bool SetObjectName(U* owner, const T& rhs, const char* objectTypeSuffix);

		DX12Context* m_ctx = nullptr;

		// DX12-specific pools
		TObjectPool<DX12PipelineStateObject>* m_PSOPool = nullptr;
		TObjectPool<DX12Semaphore>* m_SemaphorePool = nullptr;
		TObjectPool<DX12OutputMergerTarget>* m_OutputMergerTargetPool = nullptr;

		// DX12 mesh resources
		struct DX12MeshGPUResources
		{
			ComPtr<ID3D12Resource> m_VertexBuffer_Upload;
			ComPtr<ID3D12Resource> m_VertexBuffer_Default;
			ComPtr<ID3D12Resource> m_IndexBuffer_Upload;
			ComPtr<ID3D12Resource> m_IndexBuffer_Default;
			ComPtr<ID3D12Resource> m_BLAS;
			ComPtr<ID3D12Resource> m_ScratchBuffer;
		};
		std::unordered_map<uint32_t, DX12MeshGPUResources> m_DX12MeshResources;

		// DX12 texture resources
		std::unordered_map<uint64_t, ComPtr<ID3D12Resource>> m_TextureBuffers_Upload;
		std::unordered_map<uint64_t, std::vector<ComPtr<ID3D12Resource>>> m_TextureBuffers_Default;

		// Mipmap generators
		ID3D12RootSignature* m_2DMipmapRootSignature = nullptr;
		ID3D12RootSignature* m_3DMipmapRootSignature = nullptr;
		ID3D12PipelineState* m_2DMipmapPSO = nullptr;
		ID3D12PipelineState* m_3DMipmapPSO = nullptr;
	};
}
