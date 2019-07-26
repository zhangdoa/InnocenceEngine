#pragma once
#include "../IRenderingServer.h"

class DX11CommandList : public ICommandList
{
};

class DX11CommandQueue : public ICommandQueue
{
};

class DX11Semaphore : public ISemaphore
{
};

class DX11Fence : public IFence
{
};

class DX11RenderingServer : public IRenderingServer
{
	// Inherited via IRenderingServer
	virtual bool Setup() override;
	virtual bool Initialize() override;
	virtual bool Terminate() override;

	virtual ObjectStatus GetStatus() override;

	virtual MeshDataComponent * AddMeshDataComponent(const char * name) override;
	virtual TextureDataComponent * AddTextureDataComponent(const char * name) override;
	virtual MaterialDataComponent * AddMaterialDataComponent(const char * name) override;
	virtual RenderPassDataComponent * AddRenderPassDataComponent(const char * name) override;
	virtual ShaderProgramComponent * AddShaderProgramComponent(const char * name) override;
	virtual GPUBufferDataComponent * AddGPUBufferDataComponent(const char * name) override;

	virtual bool InitializeMeshDataComponent(MeshDataComponent * rhs) override;
	virtual bool InitializeTextureDataComponent(TextureDataComponent * rhs) override;
	virtual bool InitializeMaterialDataComponent(MaterialDataComponent * rhs) override;
	virtual bool InitializeRenderPassDataComponent(RenderPassDataComponent * rhs) override;
	virtual bool InitializeShaderProgramComponent(ShaderProgramComponent * rhs) override;
	virtual bool InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs) override;

	virtual bool DeleteMeshDataComponent(MeshDataComponent * rhs) override;
	virtual bool DeleteTextureDataComponent(TextureDataComponent * rhs) override;
	virtual bool DeleteMaterialDataComponent(MaterialDataComponent * rhs) override;
	virtual bool DeleteRenderPassDataComponent(RenderPassDataComponent * rhs) override;
	virtual bool DeleteShaderProgramComponent(ShaderProgramComponent * rhs) override;
	virtual bool DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs) override;

	virtual bool UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue) override;

	virtual bool CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex) override;
	virtual bool BindRenderPassDataComponent(RenderPassDataComponent * rhs) override;
	virtual bool CleanRenderTargets(RenderPassDataComponent * rhs) override;
	virtual bool BindGPUBuffer(ShaderType shaderType, GPUBufferAccessibility accessibility, GPUBufferDataComponent * GPUBufferDataComponent, size_t startOffset, size_t range) override;
	virtual bool BindShaderProgramComponent(ShaderProgramComponent * rhs) override;
	virtual bool BindMaterialDataComponent(MaterialDataComponent * rhs) override;
	virtual bool DispatchDrawCall(MeshDataComponent * rhs) override;
	virtual bool UnbindMaterialDataComponent(RenderPassDataComponent * rhs) override;
	virtual bool CommandListEnd(RenderPassDataComponent * rhs, size_t frameIndex) override;
	virtual bool ExecuteCommandList(RenderPassDataComponent * rhs, size_t frameIndex) override;
	virtual bool WaitForFrame(RenderPassDataComponent * rhs, size_t frameIndex) override;
	virtual bool Present() override;

	virtual bool CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest) override;
	virtual bool CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest) override;
	virtual bool CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex) override;

	virtual vec4 ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y) override;
	virtual std::vector<vec4> ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC) override;

	virtual bool Resize() override;

	virtual bool ReloadShader(RenderPassType renderPassType) override;
	virtual bool BakeGIData() override;
};