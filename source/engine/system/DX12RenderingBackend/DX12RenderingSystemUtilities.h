#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/MaterialDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DX12MeshDataComponent.h"
#include "../../component/DX12TextureDataComponent.h"
#include "../../component/DX12ShaderProgramComponent.h"
#include "../../component/DX12RenderPassComponent.h"

INNO_PRIVATE_SCOPE DX12RenderingSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool render();
	bool terminate();
	bool initializeComponentPool();
	bool resize();

	void loadDefaultAssets();
	bool generateGPUBuffers();

	DX12MeshDataComponent* addDX12MeshDataComponent();
	MaterialDataComponent* addMaterialDataComponent();
	DX12TextureDataComponent* addDX12TextureDataComponent();

	DX12MeshDataComponent* getDX12MeshDataComponent(EntityID meshID);
	DX12TextureDataComponent* getDX12TextureDataComponent(EntityID textureID);

	DX12MeshDataComponent* getDX12MeshDataComponent(MeshShapeType MeshShapeType);
	DX12TextureDataComponent* getDX12TextureDataComponent(TextureUsageType TextureUsageType);
	DX12TextureDataComponent* getDX12TextureDataComponent(FileExplorerIconType iconType);
	DX12TextureDataComponent* getDX12TextureDataComponent(WorldEditorIconType iconType);

	DX12RenderPassComponent* addDX12RenderPassComponent(EntityID rhs);
	bool initializeDX12RenderPassComponent(DX12RenderPassComponent* DXRPC, DX12ShaderProgramComponent* DXSPC);

	bool reserveRenderTargets(DX12RenderPassComponent* DXRPC);
	bool createRenderTargets(DX12RenderPassComponent* DXRPC);
	bool createRTVDescriptorHeap(DX12RenderPassComponent* DXRPC);
	bool createRTV(DX12RenderPassComponent* DXRPC);
	bool createRootSignature(DX12RenderPassComponent* DXRPC);
	bool createPSO(DX12RenderPassComponent* DXRPC, DX12ShaderProgramComponent* DXSPC);
	bool createCommandLists(DX12RenderPassComponent* DXRPC);
	bool createSyncPrimitives(DX12RenderPassComponent* DXRPC);

	bool initializeDX12MeshDataComponent(DX12MeshDataComponent* rhs);
	bool initializeDX12TextureDataComponent(DX12TextureDataComponent* rhs);

	bool createConstantBuffer(DX12ConstantBuffer& arg, const std::wstring& name);
	void updateConstantBufferImpl(const DX12ConstantBuffer& ConstantBuffer, size_t size, const void* ConstantBufferValue);

	template <class T>
	void updateConstantBuffer(const DX12ConstantBuffer& ConstantBuffer, const T& ConstantBufferValue)
	{
		updateConstantBufferImpl(ConstantBuffer, sizeof(T), ConstantBufferValue);
	}

	template <class T>
		void updateConstantBuffer(const DX12ConstantBuffer& ConstantBuffer, const std::vector<T>& ConstantBufferValue)
	{
		updateConstantBufferImpl(ConstantBuffer, sizeof(T) * ConstantBufferValue.size(), &ConstantBufferValue[0]);
	}

	bool recordCommandBegin(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex);
	bool recordActivateRenderPass(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex);
	bool recordBindCBV(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex, unsigned int startSlot, const DX12ConstantBuffer& ConstantBuffer);
	bool recordBindSRV(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex, unsigned int startSlot, const DX12TextureDataComponent* DXTDC);

	bool recordDrawCall(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex, DX12MeshDataComponent* DXMDC);
	bool recordCommandEnd(DX12RenderPassComponent* DXRPC, unsigned int commandListIndex);

	DX12ShaderProgramComponent* addDX12ShaderProgramComponent(EntityID rhs);

	bool initializeDX12ShaderProgramComponent(DX12ShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool activateDX12ShaderProgramComponent(DX12ShaderProgramComponent* rhs);
}