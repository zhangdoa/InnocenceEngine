#pragma once
#include "../Interface/ITimeSystem.h"

namespace Inno
{
	class InnoTimeSystem : public ITimeSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTimeSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const TimeData getCurrentTime(uint32_t timezone_adjustment = 8) override;
		const int64_t getCurrentTimeFromEpoch() override;
	};
}