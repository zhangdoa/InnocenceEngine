#include "TimeManager.h"

void TimeManager::setup()
{
}

void TimeManager::initialize()
{
	m_gameStartTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	setStatus(objectStatus::ALIVE);
}

void TimeManager::update()
{
	m_updateStartTime = std::chrono::high_resolution_clock::now();
	m_deltaTime = (std::chrono::high_resolution_clock::now() - m_updateStartTime).count();

	m_unprocessedTime += m_deltaTime;
	if (m_unprocessedTime >= m_frameTime)
	{
		m_unprocessedTime -= m_frameTime;
		setStatus(objectStatus::ALIVE);
	}
	else
	{
		setStatus(objectStatus::STANDBY);
	}
}

void TimeManager::shutdown()
{
	setStatus(objectStatus::SHUTDOWN);
}

inline std::tuple<int, unsigned, unsigned> TimeManager::getCivilFromDays(int z) noexcept
{
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
}

std::string TimeManager::getCurrentTimeInLocal(std::chrono::hours timezone_adjustment)
{

	typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type> days;
	auto now = std::chrono::system_clock::now();
	auto tp = now.time_since_epoch();

	tp += timezone_adjustment;

	days d = std::chrono::duration_cast<days>(tp);
	tp -= d;
	std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(tp);
	tp -= h;
	std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(tp);
	tp -= m;
	std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(tp);
	tp -= s;

	auto date = getCivilFromDays(d.count()); // assumes that system_clock uses
											 // 1970-01-01 0:0:0 UTC as the epoch,
											 // and does not count leap seconds.
	return std::string{ std::to_string(std::get<0>(date)) + "-" + std::to_string(std::get<1>(date)) + "-" + std::to_string(std::get<2>(date)) + " " + std::to_string(h.count()) + ":" + std::to_string(m.count()) + ":" + std::to_string(s.count()) + ":" + std::to_string(tp / std::chrono::milliseconds(1)) };
}

const __time64_t TimeManager::getGameStartTime() const
{
	return m_gameStartTime;
}

const double TimeManager::getDeltaTime() const
{
	return m_deltaTime;
}

const double TimeManager::getcurrentTime() const
{
	return (std::chrono::high_resolution_clock::now().time_since_epoch() / std::chrono::milliseconds(1));
}
