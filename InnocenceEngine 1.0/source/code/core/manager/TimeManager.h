#pragma once
#include "../interface/IEventManager.h"
#include "../manager/LogManager.h"


class TimeManager : public IEventManager
{
public:
	~TimeManager();

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static TimeManager& getInstance()
	{
		static TimeManager instance;
		return instance;
	}

	const __time64_t getGameStartTime() const;
	const double getDeltaTime() const;
	const double getcurrentTime() const;
	// Returns year/month/day triple in civil calendar
	// Preconditions:  z is number of days since 1970-01-01 and is in the range:
	//                   [numeric_limits<Int>::min(), numeric_limits<Int>::max()-719468].
	template <class Int>
	constexpr std::tuple<Int, unsigned, unsigned> civil_from_days(Int z) noexcept {
		static_assert(
			std::numeric_limits<unsigned>::digits >= 18,
			"This algorithm has not been ported to a 16 bit unsigned integer");
		static_assert(
			std::numeric_limits<Int>::digits >= 20,
			"This algorithm has not been ported to a 16 bit signed integer");
		z += 719468;
		const Int era = (z >= 0 ? z : z - 146096) / 146097;
		const unsigned doe = static_cast<unsigned>(z - era * 146097); // [0, 146096]
		const unsigned yoe =
			(doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;      // [0, 399]
		const Int y = static_cast<Int>(yoe) + era * 400;
		const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
		const unsigned mp = (5 * doy + 2) / 153;                      // [0, 11]
		const unsigned d = doy - (153 * mp + 2) / 5 + 1;              // [1, 31]
		const unsigned m = (mp < 10 ? mp + 3 : mp - 9);               // [1, 12]
		return std::tuple<Int, unsigned, unsigned>(y + (m <= 2), m, d);
	}

	template <typename Duration = std::chrono::hours>
	std::string getCurrentTimeInLocal(Duration timezone_adjustment = std::chrono::hours(8)) {

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

		auto date = civil_from_days(d.count()); // assumes that system_clock uses
												// 1970-01-01 0:0:0 UTC as the epoch,
												// and does not count leap seconds.
		return std::string{ std::to_string(std::get<0>(date)) +  "-"  + std::to_string(std::get<1>(date)) + "-" + std::to_string(std::get<2>(date)) + " " + std::to_string(h.count()) + ":" + std::to_string(m.count()) + ":" + std::to_string(s.count()) + ":" + std::to_string(tp / std::chrono::milliseconds(1)) };
	}

private:
	TimeManager();

	const double m_frameTime = 1.0 / 60.0;
	__time64_t m_gameStartTime;
	std::chrono::high_resolution_clock::time_point m_updateStartTime;
	double m_deltaTime;
	double m_unprocessedTime;
};

