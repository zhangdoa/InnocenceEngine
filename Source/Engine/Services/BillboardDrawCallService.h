#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct BillboardDrawCallServiceImpl;
	class BillboardDrawCallService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(BillboardDrawCallService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo();
		GPUBufferComponent* GetBillboardBuffer();

	private:
		BillboardDrawCallServiceImpl* m_Impl;
	};
}
