#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct DrawCallServiceImpl;
	class DrawCallService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DrawCallService);

		bool Setup(ISystemConfig* systemConfig) override;
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
