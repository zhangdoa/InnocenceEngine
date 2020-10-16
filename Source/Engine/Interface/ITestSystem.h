#pragma once
#include "ISystem.h"

class ITestSystem : public ISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITestSystem);

	virtual bool measure(const std::string& functorName, const std::function<void()>& functor) = 0;
};
