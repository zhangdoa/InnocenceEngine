#pragma once
#include "common/stdafx.h"
#include "IManager.h"

class ITimeManager : public IManager
{
public:
	virtual ~ITimeManager() {};

	virtual const __time64_t getGameStartTime() const = 0;
	virtual const double getDeltaTime() const = 0;
	virtual const double getcurrentTime() const = 0;
	template <class Int> constexpr std::tuple<Int, unsigned, unsigned>virtual getCivilFromDays(Int z) noexcept = 0;
	template <typename Duration = std::chrono::hours>virtual std::string getCurrentTimeInLocal(Duration timezone_adjustment = std::chrono::hours(8)) = 0;
};

ITimeManager* g_pTimeManager;