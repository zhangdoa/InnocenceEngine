#pragma once
#include "../../Common/Array.h"
#include "../MeshResourceService.h"
#include "../../Common/HashMap.h"
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

		uint64_t GetBLASAddress(MeshAssetHandle handle) const;

		uint32_t GetVertexSRVSlot(MeshAssetHandle handle) const;
		uint32_t GetIndexSRVSlot(MeshAssetHandle handle) const;

	protected:
		bool InitializeImpl(MeshAssetHandle handle, Inno::Array<Vertex>& vertices, Inno::Array<Index>& indices) override;
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
			uint32_t m_VertexSRVSlot = UINT32_MAX;
			uint32_t m_IndexSRVSlot  = UINT32_MAX;
		};
		Inno::HashMap<uint32_t, DX12MeshGPUResources> m_DX12MeshResources;

		DX12Context* m_ctx = nullptr;
	};
}
