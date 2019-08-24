#pragma once
#include "../IRenderingServer.h"

class DX11RenderingServer : public IRenderingServer
{
public:
	// Inherited via IRenderingServer
	bool Setup() override;
	bool Initialize() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	MeshDataComponent * AddMeshDataComponent(const char * name) override;
	TextureDataComponent * AddTextureDataComponent(const char * name) override;
	MaterialDataComponent * AddMaterialDataComponent(const char * name) override;
	RenderPassDataComponent * AddRenderPassDataComponent(const char * name) override;
	ShaderProgramComponent * AddShaderProgramComponent(const char * name) override;
	SamplerDataComponent * AddSamplerDataComponent(const char * name = "") override;
	GPUBufferDataComponent * AddGPUBufferDataComponent(const char * name) override;

	bool InitializeMeshDataComponent(MeshDataComponent * rhs) override;
	bool InitializeTextureDataComponent(TextureDataComponent * rhs) override;
	bool InitializeMaterialDataComponent(MaterialDataComponent * rhs) override;
	bool InitializeRenderPassDataComponent(RenderPassDataComponent * rhs) override;
	bool InitializeShaderProgramComponent(ShaderProgramComponent * rhs) override;
	bool InitializeSamplerDataComponent(SamplerDataComponent * rhs) override;
	bool InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs) override;

	bool DeleteMeshDataComponent(MeshDataComponent * rhs) override;
	bool DeleteTextureDataComponent(TextureDataComponent * rhs) override;
	bool DeleteMaterialDataComponent(MaterialDataComponent * rhs) override;
	bool DeleteRenderPassDataComponent(RenderPassDataComponent * rhs) override;
	bool DeleteShaderProgramComponent(ShaderProgramComponent * rhs) override;
	bool DeleteSamplerDataComponent(SamplerDataComponent * rhs) override;
	bool DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs) override;

	bool UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue, size_t startOffset, size_t range) override;

	bool CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex) override;
	bool BindRenderPassDataComponent(RenderPassDataComponent * rhs) override;
	bool CleanRenderTargets(RenderPassDataComponent * rhs) override;
	bool ActivateResourceBinder(RenderPassDataComponent * renderPass, ShaderStage shaderStage, IResourceBinder * binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount) override;
	bool DispatchDrawCall(RenderPassDataComponent * renderPass, MeshDataComponent* mesh, size_t instanceCount) override;
	bool DeactivateResourceBinder(RenderPassDataComponent * renderPass, ShaderStage shaderStage, IResourceBinder * binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount) override;
	bool CommandListEnd(RenderPassDataComponent * rhs) override;
	bool ExecuteCommandList(RenderPassDataComponent * rhs) override;
	bool WaitForFrame(RenderPassDataComponent * rhs) override;
	bool SetUserPipelineOutput(IResourceBinder* resourceBinder) override;
	bool Present() override;

	bool DispatchCompute(RenderPassDataComponent * renderPass, unsigned int threadGroupX, unsigned int threadGroupY, unsigned int threadGroupZ) override;

	bool CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest) override;
	bool CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest) override;
	bool CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex) override;

	Vec4 ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y) override;
	std::vector<Vec4> ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC) override;

	bool Resize() override;

	void* GetDevice();
	void* GetDeviceContext();
};