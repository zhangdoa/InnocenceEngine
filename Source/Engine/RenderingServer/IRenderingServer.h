#pragma once
#include "../Interface/ISystem.h"

#include "../Component/MeshDataComponent.h"
#include "../Component/TextureDataComponent.h"
#include "../Component/MaterialDataComponent.h"
#include "../Component/RenderPassDataComponent.h"
#include "../Component/ShaderProgramComponent.h"
#include "../Component/SamplerDataComponent.h"
#include "../Component/GPUBufferDataComponent.h"

namespace Inno
{
	class IRenderingServer : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingServer);

		virtual MeshDataComponent* AddMeshDataComponent(const char* name = "") = 0;
		virtual TextureDataComponent* AddTextureDataComponent(const char* name = "") = 0;
		virtual MaterialDataComponent* AddMaterialDataComponent(const char* name = "") = 0;
		virtual RenderPassDataComponent* AddRenderPassDataComponent(const char* name = "") = 0;
		virtual ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") = 0;
		virtual SamplerDataComponent* AddSamplerDataComponent(const char* name = "") = 0;
		virtual GPUBufferDataComponent* AddGPUBufferDataComponent(const char* name = "") = 0;

		virtual bool InitializeMeshDataComponent(MeshDataComponent* rhs) = 0;
		virtual bool InitializeTextureDataComponent(TextureDataComponent* rhs) = 0;
		virtual bool InitializeMaterialDataComponent(MaterialDataComponent* rhs) = 0;
		virtual bool InitializeRenderPassDataComponent(RenderPassDataComponent* rhs) = 0;
		virtual	bool InitializeShaderProgramComponent(ShaderProgramComponent* rhs) = 0;
		virtual bool InitializeSamplerDataComponent(SamplerDataComponent* rhs) = 0;
		virtual bool InitializeGPUBufferDataComponent(GPUBufferDataComponent* rhs) = 0;

		virtual	bool DeleteMeshDataComponent(MeshDataComponent* rhs) = 0;
		virtual	bool DeleteTextureDataComponent(TextureDataComponent* rhs) = 0;
		virtual	bool DeleteMaterialDataComponent(MaterialDataComponent* rhs) = 0;
		virtual	bool DeleteRenderPassDataComponent(RenderPassDataComponent* rhs) = 0;
		virtual	bool DeleteShaderProgramComponent(ShaderProgramComponent* rhs) = 0;
		virtual	bool DeleteSamplerDataComponent(SamplerDataComponent* rhs) = 0;
		virtual	bool DeleteGPUBufferDataComponent(GPUBufferDataComponent* rhs) = 0;

		virtual bool ClearTextureDataComponent(TextureDataComponent* rhs) = 0;
		virtual bool CopyTextureDataComponent(TextureDataComponent* lhs, TextureDataComponent* rhs) = 0;

	protected:
		virtual bool UploadGPUBufferDataComponentImpl(GPUBufferDataComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range) = 0;

	public:
		template<typename T>
		bool UploadGPUBufferDataComponent(GPUBufferDataComponent* rhs, const T* GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return UploadGPUBufferDataComponentImpl(rhs, GPUBufferValue, startOffset, range);
		}

		template<typename T>
		bool UploadGPUBufferDataComponent(GPUBufferDataComponent* rhs, const std::vector<T>& GPUBufferValue, size_t startOffset = 0, size_t range = SIZE_MAX)
		{
			return UploadGPUBufferDataComponentImpl(rhs, &GPUBufferValue[0], startOffset, range);
		}

		virtual bool ClearGPUBufferDataComponent(GPUBufferDataComponent* rhs) = 0;

		virtual bool CommandListBegin(RenderPassDataComponent* rhs, size_t frameIndex) = 0;
		virtual bool BindRenderPassDataComponent(RenderPassDataComponent* rhs) = 0;
		virtual bool CleanRenderTargets(RenderPassDataComponent* rhs) = 0;
		// globalSlot: The root descriptor table index in DirectX 12/The first set index in Vulkan
		// localSlot: The local type-related register slot index, like "binding = 5" in GLSL or  "t(4)"/"c(3)" in HLSL
		virtual bool ActivateResourceBinder(RenderPassDataComponent* renderPass, ShaderStage shaderStage, IResourceBinder* binder, size_t globalSlot, size_t localSlot, Accessibility accessibility = Accessibility::ReadOnly, size_t startOffset = 0, size_t elementCount = SIZE_MAX) = 0;
		virtual bool DrawIndexedInstanced(RenderPassDataComponent* renderPass, MeshDataComponent* mesh, size_t instanceCount = 1) = 0;
		virtual bool DrawInstanced(RenderPassDataComponent* renderPass, size_t instanceCount = 1) = 0;
		virtual bool DeactivateResourceBinder(RenderPassDataComponent* renderPass, ShaderStage shaderStage, IResourceBinder* binder, size_t globalSlot, size_t localSlot, Accessibility accessibility = Accessibility::ReadOnly, size_t startOffset = 0, size_t elementCount = SIZE_MAX) = 0;
		virtual bool CommandListEnd(RenderPassDataComponent* rhs) = 0;
		virtual bool ExecuteCommandList(RenderPassDataComponent* rhs) = 0;
		virtual bool WaitForFrame(RenderPassDataComponent* rhs) = 0;
		virtual bool SetUserPipelineOutput(IResourceBinder* rhs) = 0;
		virtual IResourceBinder* GetUserPipelineOutput() = 0;
		virtual bool Present() = 0;

		virtual bool Dispatch(RenderPassDataComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) = 0;

		virtual Vec4 ReadRenderTargetSample(RenderPassDataComponent* rhs, size_t renderTargetIndex, size_t x, size_t y) = 0;
		virtual std::vector<Vec4> ReadTextureBackToCPU(RenderPassDataComponent* canvas, TextureDataComponent* TDC) = 0;
		virtual bool GenerateMipmap(TextureDataComponent* rhs) = 0;

		virtual bool Resize() = 0;

		// Debug use only
		virtual bool BeginCapture() = 0;
		virtual bool EndCapture() = 0;
	};
}