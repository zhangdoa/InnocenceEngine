#pragma once
#include "../../Common/Array.h"
#include "../TextureResourceService.h"
#include "../../Common/HashMap.h"
#include "DX12Headers.h"

namespace Inno
{
	struct DX12Context;

	class DX12TextureResourceService : public TextureResourceService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DX12TextureResourceService);

		void SetDX12Context(DX12Context* ctx) { m_ctx = ctx; }

		bool Delete(TextureComponent* ptr) override;
		bool Clear(CommandListComponent* commandList, TextureComponent* texture) override;
		bool Copy(CommandListComponent* commandList, TextureComponent* src, TextureComponent* dst) override;
		bool GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList = nullptr) override;
		std::optional<uint32_t> GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) override;
		Inno::Array<Math::Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* textureComp) override;

		bool CreateMipmapGenerator();
		bool ReleaseMipmapGenerator();

	protected:
		bool InitializeImpl(TextureComponent* texture, void* textureData) override;

	private:
		bool CreateSRV(TextureComponent* texture, uint32_t mostDetailedMip);
		bool CreateUAV(TextureComponent* texture, uint32_t mipSlice);

		DX12Context* m_ctx = nullptr;

		Inno::HashMap<uint64_t, ComPtr<ID3D12Resource>> m_TextureBuffers_Upload;
		Inno::HashMap<uint64_t, Inno::Array<ComPtr<ID3D12Resource>>> m_TextureBuffers_Default;

		ID3D12RootSignature* m_2DMipmapRootSignature = nullptr;
		ID3D12RootSignature* m_3DMipmapRootSignature = nullptr;
		ID3D12PipelineState* m_2DMipmapPSO = nullptr;
		ID3D12PipelineState* m_3DMipmapPSO = nullptr;
	};
}
