#pragma once
#include "../Interface/ISystem.h"
#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct DebugDrawCallServiceImpl;
	class DebugDrawCallService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(DebugDrawCallService);

		bool Setup(ISystemConfig* systemConfig) override;
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
