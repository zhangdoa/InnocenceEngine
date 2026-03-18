#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct PerFrameDataServiceImpl;
	class PerFrameDataService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(PerFrameDataService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const PerFrameConstantBuffer& GetPerFrameConstantBuffer();
		GPUBufferComponent* GetCurrentFrameBuffer();
		GPUBufferComponent* GetPreviousFrameBuffer();

	private:
		PerFrameDataServiceImpl* m_Impl;
	};
}
