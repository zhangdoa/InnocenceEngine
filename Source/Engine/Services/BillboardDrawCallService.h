#pragma once
#include "../Interface/IService.h"
#include "../Services/IGraphicsService.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct BillboardDrawCallServiceImpl;
	class BillboardDrawCallService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(BillboardDrawCallService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		void OnSceneLoaded();

		const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo();
		GPUBufferComponent* GetBillboardBuffer();

	private:
		BillboardDrawCallServiceImpl* m_Impl;
	};
}
