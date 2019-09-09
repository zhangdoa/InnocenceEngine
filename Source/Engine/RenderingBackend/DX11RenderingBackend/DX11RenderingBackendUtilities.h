#pragma once
#include "../../Common/InnoType.h"

#include "../../Component/DX11MeshDataComponent.h"
#include "../../Component/DX11TextureDataComponent.h"
#include "../../Component/DX11MaterialDataComponent.h"
#include "../../Component/DX11RenderPassComponent.h"
#include "../../Component/DX11ShaderProgramComponent.h"
#include "../../Component/DX11RenderingBackendComponent.h"

namespace DX11RenderingBackendNS
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

	DX11MeshDataComponent* addDX11MeshDataComponent();
	DX11TextureDataComponent* addDX11TextureDataComponent();
	DX11MaterialDataComponent* addDX11MaterialDataComponent();

	bool initializeDX11MeshDataComponent(DX11MeshDataComponent* rhs);
	bool initializeDX11TextureDataComponent(DX11TextureDataComponent* rhs);
	bool initializeDX11MaterialDataComponent(DX11MaterialDataComponent* rhs);

	DX11MeshDataComponent* getDX11MeshDataComponent(MeshShapeType MeshShapeType);
	DX11TextureDataComponent* getDX11TextureDataComponent(TextureUsageType TextureUsageType);
	DX11TextureDataComponent* getDX11TextureDataComponent(FileExplorerIconType iconType);
	DX11TextureDataComponent* getDX11TextureDataComponent(WorldEditorIconType iconType);

	DX11RenderPassComponent* addDX11RenderPassComponent(const EntityID& parentEntity, const char* name);
	bool initializeDX11RenderPassComponent(DX11RenderPassComponent* DXRPC);

	bool reserveRenderTargets(DX11RenderPassComponent* DXRPC);
	bool createRenderTargets(DX11RenderPassComponent* DXRPC);
	bool createRTV(DX11RenderPassComponent* DXRPC);
	bool createDSV(DX11RenderPassComponent* DXRPC);
	bool setupPipeline(DX11RenderPassComponent* DXRPC);

	DX11ShaderProgramComponent* addDX11ShaderProgramComponent(EntityID rhs);
	bool initializeDX11ShaderProgramComponent(DX11ShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	DX11ConstantBuffer createConstantBuffer(size_t elementSize, size_t elementCount, const std::string& name);
	bool createStructuredBuffer(void* initialData, DX11StructuredBuffer& arg, const std::string& name);

	bool destroyStructuredBuffer(DX11StructuredBuffer& arg);

	void activateRenderPass(DX11RenderPassComponent * DXRPC);
	bool activateShader(DX11ShaderProgramComponent* rhs);

	void cleanRTV(vec4 color, ID3D11RenderTargetView* RTV);
	void cleanDSV(ID3D11DepthStencilView* DSV);

	void updateConstantBufferImpl(const DX11ConstantBuffer& ConstantBuffer, size_t size, const void* ConstantBufferValue);

	template <class T>
	void updateConstantBuffer(const DX11ConstantBuffer& ConstantBuffer, const T& ConstantBufferValue)
	{
		updateConstantBufferImpl(ConstantBuffer, sizeof(T), &ConstantBufferValue);
	}

	template <class T>
	void updateConstantBuffer(const DX11ConstantBuffer& ConstantBuffer, const std::vector<T>& ConstantBufferValue)
	{
		updateConstantBufferImpl(ConstantBuffer, sizeof(T) * ConstantBufferValue.size(), &ConstantBufferValue[0]);
	}

	void bindConstantBuffer(ShaderType shaderType, uint32_t startSlot, const DX11ConstantBuffer& ConstantBuffer);
	void bindConstantBuffer(ShaderType shaderType, uint32_t startSlot, const DX11ConstantBuffer& ConstantBuffer, uint32_t offset);

	void bindStructuredBufferForWrite(ShaderType shaderType, uint32_t startSlot, const DX11StructuredBuffer& StructuredBuffer);
	void bindStructuredBufferForRead(ShaderType shaderType, uint32_t startSlot, const DX11StructuredBuffer& StructuredBuffer);
	void unbindStructuredBufferForWrite(ShaderType shaderType, uint32_t startSlot);
	void unbindStructuredBufferForRead(ShaderType shaderType, uint32_t startSlot);
	void bindTextureForWrite(ShaderType shaderType, uint32_t startSlot, DX11TextureDataComponent* DXTDC);
	void bindTextureForRead(ShaderType shaderType, uint32_t startSlot, DX11TextureDataComponent* DXTDC);
	void unbindTextureForWrite(ShaderType shaderType, uint32_t startSlot);
	void unbindTextureForRead(ShaderType shaderType, uint32_t startSlot);
	void drawMesh(DX11MeshDataComponent * DXMDC);
}