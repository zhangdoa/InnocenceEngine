#pragma once
#include "../Interface/IService.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct PerFrameDataServiceImpl;
	class PerFrameDataService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(PerFrameDataService);

		bool Setup(IServiceConfig* systemConfig) override;
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
