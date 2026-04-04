#pragma once
#include "../FrameManagementService.h"

namespace Inno
{
	class IGraphicsService;

	class DX12FrameManagementService : public FrameManagementService
	{
	public:
		void SetBackend(IGraphicsService* backend) { m_Backend = backend; }

		uint32_t GetCurrentFrame() override;
		uint32_t GetPreviousFrame() override;
		uint32_t GetNextFrame() override;
		uint32_t GetSwapChainImageCount() override;
		uint32_t GetFrameCountSinceLaunch() override;

		void SetUploadHeapPreparationCallback(std::function<bool()>&& callback) override;
		void SetCommandPreparationCallback(std::function<bool()>&& callback) override;
		void SetCommandExecutionCallback(std::function<bool()>&& callback) override;

		RenderPassComponent* GetSwapChainRenderPassComponent() override;
		bool Resize() override;
		bool Present() override;

		bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& func) override;
		GPUResourceComponent* GetUserPipelineOutput() override;

	private:
		IGraphicsService* m_Backend = nullptr;
	};
}
