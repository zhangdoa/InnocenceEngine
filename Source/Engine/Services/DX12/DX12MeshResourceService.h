#pragma once
#include "../MeshResourceService.h"
#include "DX12Headers.h"

namespace Inno
{
	struct DX12Context;

	class DX12MeshResourceService : public MeshResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12MeshResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

		bool Delete(MeshComponent* mesh) override;

	protected:
		bool InitializeImpl(MeshAssetHandle handle, std::vector<Vertex>& vertices, std::vector<Index>& indices) override;
		void ReleaseMeshGPUResourceImpl(MeshAssetHandle handle) override;

	private:
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

		DX12Context* m_ctx = nullptr;
	};
}
