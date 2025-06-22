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
		IPipelineStateObject* AddPipelineStateObject() override;
		ISemaphore* AddSemaphore() override;
		bool Add(IOutputMergerTarget*& rhs) override;

		// Delete operations - all return true (success)
		bool Delete(MeshComponent* mesh) override;
		bool Delete(TextureComponent* texture) override;
		bool Delete(MaterialComponent* material) override;
		bool Delete(RenderPassComponent* renderPass) override;
		bool Delete(ShaderProgramComponent* shaderProgram) override;
		bool Delete(SamplerComponent* sampler) override;
		bool Delete(GPUBufferComponent* gpuBuffer) override;
		bool Delete(IPipelineStateObject* rhs) override;
		bool Delete(CommandListComponent* rhs) override;
		bool Delete(ISemaphore* rhs) override;
		bool Delete(IOutputMergerTarget* rhs) override;

		// Most rendering operations are no-ops
		bool CommandListBegin(RenderPassComponent* renderPass, CommandListComponent* commandList, size_t frameIndex) override;
		bool CommandListEnd(RenderPassComponent* renderPass, CommandListComponent* commandList) override;
		bool Execute(CommandListComponent* commandList, GPUEngineType GPUEngineType) override;
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
