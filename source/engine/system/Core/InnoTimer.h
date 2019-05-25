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

class InnoTimer
{
public:
	static bool Setup();
	static bool Initialize();
	static bool Tick();
	static bool Terminate();

	static const unsigned long long GetCurrentTimeFromEpoch(TimeUnit time_unit);
	static const Timestamp GetCurrentTime(unsigned int time_zone_adjustment);
	static const double GetDeltaTime(TimeUnit time_unit);
};
