#pragma once
#include <chrono>
#include <ctime>
#include "ISystem.h"

class ITimeSystem : public ISystem
{
public:
	virtual ~ITimeSystem() {};

	virtual const time_t getGameStartTime() const = 0;
	virtual const long long getDeltaTime() const = 0;
	virtual const long long getcurrentTime() const = 0;
	virtual const std::tuple<int, unsigned, unsigned> getCivilFromDays(int z) const = 0;
	virtual const std::string getCurrentTimeInLocal(std::chrono::hours timezone_adjustment = std::chrono::hours(8)) const = 0;
	virtual const std::string getCurrentTimeInLocalForOutput(std::chrono::hours timezone_adjustment = std::chrono::hours(8)) const = 0;
};
