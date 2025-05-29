#pragma once
#include "../IRenderingServer.h"

namespace Inno
{
	class HeadlessRenderingServer : public IRenderingServer
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(HeadlessRenderingServer);

		bool Setup(ISystemConfig* systemConfig = nullptr) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;
		std::vector<std::type_index> GetDependencies() override;

		// Component Pool APIs - return null/dummy components
		MeshComponent* AddMeshComponent(const char* name = "") override;
		TextureComponent* AddTextureComponent(const char* name = "") override;
		MaterialComponent* AddMaterialComponent(const char* name = "") override;
		RenderPassComponent* AddRenderPassComponent(const char* name = "") override;
		ShaderProgramComponent* AddShaderProgramComponent(const char* name = "") override;
		SamplerComponent* AddSamplerComponent(const char* name = "") override;
		GPUBufferComponent* AddGPUBufferComponent(const char* name = "") override;
		IPipelineStateObject* AddPipelineStateObject() override;
		ICommandList* AddCommandList() override;
		ISemaphore* AddSemaphore() override;
		bool Add(IOutputMergerTarget*& rhs) override;

		// Delete operations - all return true (success)
		bool Delete(MeshComponent* rhs) override;
		bool Delete(TextureComponent* rhs) override;
		bool Delete(MaterialComponent* rhs) override;
		bool Delete(RenderPassComponent* rhs) override;
		bool Delete(ShaderProgramComponent* rhs) override;
		bool Delete(SamplerComponent* rhs) override;
		bool Delete(GPUBufferComponent* rhs) override;
		bool Delete(IPipelineStateObject* rhs) override;
		bool Delete(ICommandList* rhs) override;
		bool Delete(ISemaphore* rhs) override;
		bool Delete(IOutputMergerTarget* rhs) override;

		// Most rendering operations are no-ops
		bool CommandListBegin(RenderPassComponent* rhs, size_t frameIndex) override;
		bool CommandListEnd(RenderPassComponent* rhs) override;
		bool ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType) override;
		bool Present() override;

	protected:
		bool CreateHardwareResources() override;
		bool ReleaseHardwareResources() override;
		bool GetSwapChainImages() override;
		bool AssignSwapChainImages() override;
		bool ReleaseSwapChainImages() override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	};
}
