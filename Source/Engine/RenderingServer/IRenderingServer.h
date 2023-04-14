#pragma once
#include "../Interface/ISystem.h"

#include "../Component/MeshComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/RenderPassComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/SamplerComponent.h"
#include "../Component/GPUBufferComponent.h"

namespace Inno
{
	class IRenderingServer : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingServer);

		virtual MeshComponent* AddMeshComponent(const char* name = "") = 0;
		virtual TextureComponent* AddTextureComponent(const char* name = "") = 0;
		virtual MaterialComponent* AddMaterialComponent(const char* name = "") = 0;
		virtual RenderPassComponent* AddRenderPassComponent(const char* name = "") = 0;
		virtual ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") = 0;
		virtual SamplerComponent* AddSamplerComponent(const char* name = "") = 0;
		virtual GPUBufferComponent* AddGPUBufferComponent(const char* name = "") = 0;

		virtual bool InitializeMeshComponent(MeshComponent* rhs) = 0;
		virtual bool InitializeTextureComponent(TextureComponent* rhs) = 0;
		virtual bool InitializeMaterialComponent(MaterialComponent* rhs) = 0;
		virtual bool InitializeRenderPassComponent(RenderPassComponent* rhs) = 0;
		virtual	bool InitializeShaderProgramComponent(ShaderProgramComponent* rhs) = 0;
		virtual bool InitializeSamplerComponent(SamplerComponent* rhs) = 0;
		virtual bool InitializeGPUBufferComponent(GPUBufferComponent* rhs) = 0;

		virtual	bool DeleteMeshComponent(MeshComponent* rhs) = 0;
		virtual	bool DeleteTextureComponent(TextureComponent* rhs) = 0;
		virtual	bool DeleteMaterialComponent(MaterialComponent* rhs) = 0;
		virtual	bool DeleteRenderPassComponent(RenderPassComponent* rhs) = 0;
		virtual	bool DeleteShaderProgramComponent(ShaderProgramComponent* rhs) = 0;
		virtual	bool DeleteSamplerComponent(SamplerComponent* rhs) = 0;
		virtual	bool DeleteGPUBufferComponent(GPUBufferComponent* rhs) = 0;

		virtual bool UpdateMeshComponent(MeshComponent* rhs) = 0;

		virtual bool ClearTextureComponent(TextureComponent* rhs) = 0;
		virtual bool CopyTextureComponent(TextureComponent* lhs, TextureComponent* rhs) = 0;

	protected:
		virtual bool UploadGPUBufferComponentImpl(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range) = 0;

	public:
		template<typename T>
		bool UploadGPUBufferComponent(GPUBufferComponent* rhs, const T* GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return UploadGPUBufferComponentImpl(rhs, GPUBufferValue, startOffset, range);
		}

		template<typename T>
		bool UploadGPUBufferComponent(GPUBufferComponent* rhs, const std::vector<T>& GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return UploadGPUBufferComponentImpl(rhs, &GPUBufferValue[0], startOffset, range);
		}

		virtual bool ClearGPUBufferComponent(GPUBufferComponent* rhs) = 0;

		virtual bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex) = 0;
		virtual bool BindRenderPassComponent(RenderPassComponent* rhs) = 0;
		virtual bool CleanRenderTargets(RenderPassComponent* rhs) = 0;
		virtual bool BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, Accessibility accessibility = Accessibility::ReadOnly, size_t startOffset = 0, size_t elementCount = SIZE_MAX) = 0;
		virtual bool DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount = 1) = 0;
		virtual bool DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount = 1) = 0;
		virtual bool UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, Accessibility accessibility = Accessibility::ReadOnly, size_t startOffset = 0, size_t elementCount = SIZE_MAX) = 0;
		virtual bool CommandListEnd(RenderPassComponent* rhs) = 0;
		virtual bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType) = 0;
		virtual bool WaitCommandQueue(RenderPassComponent* rhs, GPUEngineType queueType, GPUEngineType semaphoreType) = 0;
		virtual bool WaitFence(RenderPassComponent* rhs, GPUEngineType GPUEngineType) = 0;
		virtual bool SetUserPipelineOutput(GPUResourceComponent* rhs) = 0;
		virtual GPUResourceComponent* GetUserPipelineOutput() = 0;
		virtual bool Present() = 0;

		virtual bool Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) = 0;

		virtual Vec4 ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) = 0;
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp) = 0;
		virtual bool GenerateMipmap(TextureComponent* rhs) = 0;

		virtual bool Resize() = 0;

		// Debug use only
		virtual bool BeginCapture() = 0;
		virtual bool EndCapture() = 0;
	};
}