#pragma once
#include "../Interface/IService.h"
#include "../Common/GraphicsPrimitive.h"

namespace Inno
{
	class RenderPassComponent;
	struct GPUResourceComponent;

	class FrameManagementService : public IService
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(FrameManagementService);

		bool Setup(IServiceConfig* systemConfig = nullptr) override { return true; }
		bool Initialize() override { return true; }
		bool Update() override { return true; }
		bool Terminate() override { return true; }
		ObjectStatus GetStatus() override { return ObjectStatus::Activated; }

		// Frame queries
		virtual uint32_t GetCurrentFrame() = 0;
		virtual uint32_t GetPreviousFrame() = 0;
		virtual uint32_t GetNextFrame() = 0;
		virtual uint32_t GetSwapChainImageCount() = 0;
		virtual uint32_t GetFrameCountSinceLaunch() = 0;

		// Callbacks from Engine
		virtual void SetUploadHeapPreparationCallback(std::function<bool()>&& callback) = 0;
		virtual void SetCommandPreparationCallback(std::function<bool()>&& callback) = 0;
		virtual void SetCommandExecutionCallback(std::function<bool()>&& callback) = 0;

		// Swap chain
		virtual RenderPassComponent* GetSwapChainRenderPassComponent() = 0;
		virtual bool Resize() = 0;
		virtual bool Present() = 0;

		// User pipeline output
		virtual bool SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& func) = 0;
		virtual GPUResourceComponent* GetUserPipelineOutput() = 0;
	};
}
