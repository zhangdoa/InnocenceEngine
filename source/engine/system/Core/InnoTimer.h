#pragma once
enum class TimeUnit { Microsecond, Millisecond, Second, Minute, Hour, Day, Month, Year };

struct Timestamp
{
	unsigned short Year;
	unsigned short Month;
	unsigned short Day;
	unsigned short Hour;
	unsigned short Minute;
	unsigned short Second;
	unsigned short Millisecond;
	unsigned short Microsecond;
};

namespace InnoTimer
{
	bool Setup();
	bool Initialize();
	bool Tick();
	bool Terminate();

	const unsigned long long GetCurrentTimeFromEpoch(TimeUnit time_unit);
	const Timestamp GetCurrentTime(unsigned int time_zone_adjustment);
	const double GetDeltaTime(TimeUnit time_unit);
};
