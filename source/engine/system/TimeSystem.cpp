#include "TimeSystem.h"
#include <chrono>
#include <ctime>
#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoTimeSystemNS
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	const double m_frameTime = (1.0 / 120.0) * 1000.0 * 1000.0;
	long long m_gameStartTime;
	std::chrono::high_resolution_clock::time_point m_updateStartTime;
	long long m_deltaTime;
	double m_unprocessedTime;
};

INNO_SYSTEM_EXPORT bool InnoTimeSystem::setup()
{
	InnoTimeSystemNS::m_gameStartTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTimeSystem::initialize()
{
	InnoTimeSystemNS::m_objectStatus = objectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog("TimeSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTimeSystem::update()
{
	InnoTimeSystemNS::m_updateStartTime = std::chrono::high_resolution_clock::now();
	InnoTimeSystemNS::m_deltaTime = (std::chrono::high_resolution_clock::now() - InnoTimeSystemNS::m_updateStartTime).count();

	InnoTimeSystemNS::m_unprocessedTime += InnoTimeSystemNS::m_deltaTime;
	if (InnoTimeSystemNS::m_unprocessedTime >= InnoTimeSystemNS::m_frameTime)
	{
		InnoTimeSystemNS::m_unprocessedTime -= InnoTimeSystemNS::m_frameTime;
		InnoTimeSystemNS::m_objectStatus = objectStatus::ALIVE;
	}
	else
	{
		InnoTimeSystemNS::m_objectStatus = objectStatus::STANDBY;
	}
	return true;
}

INNO_SYSTEM_EXPORT bool InnoTimeSystem::terminate()
{
	InnoTimeSystemNS::m_objectStatus = objectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog("TimeSystem has been terminated.");
	return true;
}

INNO_SYSTEM_EXPORT const std::tuple<int, unsigned, unsigned> InnoTimeSystem::getCivilFromDays(int z)
{
	static_assert(
		std::numeric_limits<unsigned>::digits >= 18,
		"This algorithm has not been ported to a 16 bit unsigned integer");
	static_assert(
		std::numeric_limits<int>::digits >= 20,
		"This algorithm has not been ported to a 16 bit signed integer");
	z += 719468;
	const int era = (z >= 0 ? z : z - 146096) / 146097;
	const unsigned doe = static_cast<unsigned>(z - era * 146097); // [0, 146096]
	const unsigned yoe =
		(doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;      // [0, 399]
	const int y = static_cast<int>(yoe) + era * 400;
	const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
	const unsigned mp = (5 * doy + 2) / 153;                      // [0, 11]
	const unsigned d = doy - (153 * mp + 2) / 5 + 1;              // [1, 31]
	const unsigned m = (mp < 10 ? mp + 3 : mp - 9);               // [1, 12]
	return std::tuple<int, unsigned, unsigned>(y + (m <= 2), m, d);
}

INNO_SYSTEM_EXPORT const timeData InnoTimeSystem::getCurrentTimeInLocal(unsigned int timezone_adjustment)
{
	typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type> days;
	auto now = std::chrono::system_clock::now();
	auto tp = now.time_since_epoch();

	tp += std::chrono::hours(timezone_adjustment);

	days d = std::chrono::duration_cast<days>(tp);
	tp -= d;
	std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(tp);
	tp -= h;
	std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(tp);
	tp -= m;
	std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(tp);
	tp -= s;

	auto date = getCivilFromDays(d.count());
	// assumes that system_clock uses
	// 1970-01-01 0:0:0 UTC as the epoch,
	// and does not count leap seconds.

	timeData l_timeData;

	l_timeData.year = std::get<0>(date);
	l_timeData.month = std::get<1>(date);
	l_timeData.day = std::get<2>(date);
	l_timeData.hour = h.count();
	l_timeData.minute = m.count();
	l_timeData.second = s.count();
	l_timeData.millisecond = tp / std::chrono::milliseconds(1);

	return  l_timeData;
}

INNO_SYSTEM_EXPORT objectStatus InnoTimeSystem::getStatus()
{
	return InnoTimeSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT const long long InnoTimeSystem::getStartTime()
{
	return InnoTimeSystemNS::m_gameStartTime;
}

INNO_SYSTEM_EXPORT const long long InnoTimeSystem::getDeltaTime()
{
	return InnoTimeSystemNS::m_deltaTime;
}

INNO_SYSTEM_EXPORT const long long InnoTimeSystem::getCurrentTime()
{
	return (std::chrono::high_resolution_clock::now().time_since_epoch() / std::chrono::milliseconds(1));
}
