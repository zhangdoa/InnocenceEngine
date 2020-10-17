#pragma once
#include "ISystem.h"

namespace Inno
{
	class ITimeSystem : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ITimeSystem);

		virtual const TimeData getCurrentTime(uint32_t timezone_adjustment = 8) = 0;
		virtual const int64_t getCurrentTimeFromEpoch() = 0;
	};
}