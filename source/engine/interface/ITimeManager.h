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
	virtual inline std::tuple<int, unsigned, unsigned> getCivilFromDays(int z) noexcept = 0;
	virtual std::string getCurrentTimeInLocal(std::chrono::hours timezone_adjustment = std::chrono::hours(8)) = 0;
};