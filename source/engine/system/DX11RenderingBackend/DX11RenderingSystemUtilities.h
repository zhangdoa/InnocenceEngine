#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/MaterialDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/DX11MeshDataComponent.h"
#include "../../component/DX11TextureDataComponent.h"
#include "../../component/DX11RenderPassComponent.h"
#include "../../component/DX11ShaderProgramComponent.h"
#include "../../component/DX11RenderingSystemComponent.h"

INNO_PRIVATE_SCOPE DX11RenderingSystemNS
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
	MaterialDataComponent* addMaterialDataComponent();
	DX11TextureDataComponent* addDX11TextureDataComponent();

	bool initializeDX11MeshDataComponent(DX11MeshDataComponent* rhs);
	bool initializeDX11TextureDataComponent(DX11TextureDataComponent* rhs);

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

	void bindConstantBuffer(ShaderType shaderType, unsigned int startSlot, const DX11ConstantBuffer& ConstantBuffer);
	void bindConstantBuffer(ShaderType shaderType, unsigned int startSlot, const DX11ConstantBuffer& ConstantBuffer, unsigned int offset);

	void bindStructuredBufferForWrite(ShaderType shaderType, unsigned int startSlot, const DX11StructuredBuffer& StructuredBuffer);
	void bindStructuredBufferForRead(ShaderType shaderType, unsigned int startSlot, const DX11StructuredBuffer& StructuredBuffer);
	void unbindStructuredBufferForWrite(ShaderType shaderType, unsigned int startSlot);
	void unbindStructuredBufferForRead(ShaderType shaderType, unsigned int startSlot);
	void bindTextureForWrite(ShaderType shaderType, unsigned int startSlot, DX11TextureDataComponent* DXTDC);
	void bindTextureForRead(ShaderType shaderType, unsigned int startSlot, DX11TextureDataComponent* DXTDC);
	void unbindTextureForWrite(ShaderType shaderType, unsigned int startSlot);
	void unbindTextureForRead(ShaderType shaderType, unsigned int startSlot);
	void drawMesh(DX11MeshDataComponent * DXMDC);
}