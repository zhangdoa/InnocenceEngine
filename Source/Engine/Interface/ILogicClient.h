#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Common/ComponentHeaders.h"

class ILogicClient
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ILogicClient);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual std::string getApplicationName() = 0;
};
