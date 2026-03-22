#pragma once
#include "../Interface/IService.h"
#include "../Services/IGraphicsService.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct DrawCallServiceImpl;
	class DrawCallService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DrawCallService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const std::vector<GPUModelData>& GetGPUModelData();
		GPUBufferComponent* GetGPUModelDataBuffer();
		GPUBufferComponent* GetCurrentFrameTransformBuffer();
		GPUBufferComponent* GetPreviousFrameTransformBuffer();
		GPUBufferComponent* GetMaterialBuffer();

	private:
		DrawCallServiceImpl* m_Impl;
	};
}
