#pragma once
#include "../Common/STL14.h"

namespace Inno
{
	using HRClock = std::chrono::high_resolution_clock;
	using Days = std::chrono::duration<int32_t, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type>;
	using Months = std::chrono::duration<int32_t, std::ratio_multiply<std::chrono::hours::period, std::ratio<744>>::type>;
	using Years = std::chrono::duration<int32_t, std::ratio_multiply<std::chrono::hours::period, std::ratio<8760>>::type>;

	enum class TimeUnit { Microsecond, Millisecond, Second, Minute, Hour, Day, Month, Year };

	struct Timestamp
	{
		uint32_t Year;
		uint32_t Month;
		uint32_t Day;
		uint32_t Hour;
		uint32_t Minute;
		uint32_t Second;
		uint32_t Millisecond;
		uint32_t Microsecond;
	};

	class Timer
	{
	public:
		Timer();
		void Tick();

		static uint64_t GetCurrentTimeFromEpoch(TimeUnit time_unit = TimeUnit::Millisecond);
		static Timestamp GetCurrentTime(uint32_t time_zone_adjustment = 0);

		static bool Measure(const std::string& functorName, const std::function<void()>& functor);

	private:
		double m_FrameLength;
		HRClock::time_point m_EngineStartTime;
		HRClock::time_point m_UpdateStartTime;
		double m_DeltaTime;
		double m_UnprocessedTime;
	};
}