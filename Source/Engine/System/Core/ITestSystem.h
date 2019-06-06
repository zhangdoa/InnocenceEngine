#pragma once
#include "../../Common/InnoType.h"

#include "../../Common/InnoClassTemplate.h"

INNO_INTERFACE ITestSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITestSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool measure(const std::string& functorName, const std::function<void()>& functor) = 0;
};
