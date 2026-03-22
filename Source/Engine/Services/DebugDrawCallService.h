#pragma once
#include "../Interface/IService.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct DebugDrawCallServiceImpl;
	class DebugDrawCallService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DebugDrawCallService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo();
		void Submit(const DebugPassDrawCallInfo& info);

	private:
		DebugDrawCallServiceImpl* m_Impl;
	};
}
