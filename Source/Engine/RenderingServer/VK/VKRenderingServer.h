#pragma once
#include "../IRenderingServer.h"

class VKCommandList : public ICommandList
{
};

class VKCommandQueue : public ICommandQueue
{
};

class VKSemaphore : public ISemaphore
{
};

class VKFence : public IFence
{
};

class VKRenderingServer : public IRenderingServer
{
	// Inherited via IRenderingServer
	virtual bool Setup() override;
	virtual bool Initialize() override;
	virtual bool Terminate() override;

	virtual ObjectStatus GetStatus() override;

	virtual MeshDataComponent * AddMeshDataComponent(const char * name) override;
	virtual TextureDataComponent * AddTextureDataComponent(const char * name) override;
	virtual MaterialDataComponent * AddMaterialDataComponent(const char * name) override;
	virtual RenderPassComponent * AddRenderPassComponent(const char * name) override;
	virtual ShaderProgramComponent * AddShaderProgramComponent(const char * name) override;
	virtual GPUBufferDataComponent * AddGPUBufferDataComponent(const char * name) override;

	virtual bool InitializeMeshDataComponent(MeshDataComponent * rhs) override;
	virtual bool InitializeTextureDataComponent(TextureDataComponent * rhs) override;
	virtual bool InitializeMaterialDataComponent(MaterialDataComponent * rhs) override;
	virtual bool InitializeRenderPassComponent(RenderPassComponent * rhs) override;
	virtual bool InitializeShaderProgramComponent(ShaderProgramComponent * rhs) override;
	virtual bool InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs) override;

	virtual bool DeleteMeshDataComponent(MeshDataComponent * rhs) override;
	virtual bool DeleteTextureDataComponent(TextureDataComponent * rhs) override;
	virtual bool DeleteMaterialDataComponent(MaterialDataComponent * rhs) override;
	virtual bool DeleteRenderPassComponent(RenderPassComponent * rhs) override;
	virtual bool DeleteShaderProgramComponent(ShaderProgramComponent * rhs) override;

	virtual void RegisterMeshDataComponent(MeshDataComponent * rhs) override;
	virtual void RegisterMaterialDataComponent(MaterialDataComponent * rhs) override;

	virtual MeshDataComponent * GetMeshDataComponent(MeshShapeType meshShapeType) override;
	virtual TextureDataComponent * GetTextureDataComponent(TextureUsageType textureUsageType) override;
	virtual TextureDataComponent * GetTextureDataComponent(WorldEditorIconType iconType) override;

	virtual bool UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue) override;

	virtual bool BindRenderPassComponent(RenderPassComponent * rhs) override;
	virtual bool CleanRenderTargets(RenderPassComponent * rhs) override;
	virtual bool BindGPUBuffer(ShaderType shaderType, GPUBufferAccessibility accessibility, GPUBufferDataComponent * GPUBufferDataComponent, size_t startOffset, size_t range) override;
	virtual bool BindShaderProgramComponent(ShaderProgramComponent * rhs) override;
	virtual bool BindMaterialDataComponent(MaterialDataComponent * rhs) override;
	virtual bool DispatchDrawCall(MeshDataComponent * rhs) override;
	virtual bool UnbindMaterialDataComponent(RenderPassComponent * rhs) override;
	virtual bool CommandListEnd(RenderPassComponent * rhs, size_t frameIndex) override;
	virtual bool ExecuteCommandList(RenderPassComponent * rhs, size_t frameIndex) override;
	virtual bool WaitForFrame(RenderPassComponent * rhs, size_t frameIndex) override;
	virtual bool Present() override;

	virtual bool CopyDepthBuffer(RenderPassComponent * src, RenderPassComponent * dest) override;
	virtual bool CopyStencilBuffer(RenderPassComponent * src, RenderPassComponent * dest) override;
	virtual bool CopyColorBuffer(RenderPassComponent * src, size_t srcIndex, RenderPassComponent * dest, size_t destIndex) override;

	virtual vec4 ReadRenderTargetSample(RenderPassComponent * rhs, size_t renderTargetIndex, size_t x, size_t y) override;
	virtual std::vector<vec4> ReadTextureBackToCPU(RenderPassComponent * canvas, TextureDataComponent * TDC) override;

	virtual bool Resize() override;

	virtual bool ReloadShader(RenderPassType renderPassType) override;
	virtual bool BakeGIData() override;
};